//===--- Rewriter.cpp - Code rewriting interface --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Rewriter class, which is used for code
//  transformations.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Decl.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "ParallelRewriter.h"
using namespace clang;

raw_ostream & parallel::RewriteBuffer::write(raw_ostream &os) const {
  // FIXME: eliminate the copy by writing out each chunk at a time
  os << std::string(begin(), end());
  return os;
}

/// \brief Return true if this character is non-new-line whitespace:
/// ' ', '\\t', '\\f', '\\v', '\\r'.
static inline bool isWhitespace(unsigned char c) {
  switch (c) {
  case ' ':
  case '\t':
  case '\f':
  case '\v':
  case '\r':
    return true;
  default:
    return false;
  }
}

void parallel::RewriteBuffer::RemoveText(unsigned OrigOffset, unsigned Size,
                               bool removeLineIfEmpty) {
  // Nothing to remove, exit early.
  if (Size == 0) return;

  unsigned RealOffset = getMappedOffset(OrigOffset, true);
  assert(RealOffset+Size < Buffer.size() && "Invalid location");

  // Remove the dead characters.
  Buffer.erase(RealOffset, Size);

  // Add a delta so that future changes are offset correctly.
  AddReplaceDelta(OrigOffset, -Size);

  if (removeLineIfEmpty) {
    // Find the line that the remove occurred and if it is completely empty
    // remove the line as well.

    iterator curLineStart = begin();
    unsigned curLineStartOffs = 0;
    iterator posI = begin();
    for (unsigned i = 0; i != RealOffset; ++i) {
      if (*posI == '\n') {
        curLineStart = posI;
        ++curLineStart;
        curLineStartOffs = i + 1;
      }
      ++posI;
    }
  
    unsigned lineSize = 0;
    posI = curLineStart;
    while (posI != end() && isWhitespace(*posI)) {
      ++posI;
      ++lineSize;
    }
    if (posI != end() && *posI == '\n') {
      Buffer.erase(curLineStartOffs, lineSize + 1/* + '\n'*/);
      AddReplaceDelta(curLineStartOffs, -(lineSize + 1/* + '\n'*/));
    }
  }
}

void parallel::RewriteBuffer::InsertText(unsigned OrigOffset, StringRef Str,
                               bool InsertAfter) {

  // Nothing to insert, exit early.
  if (Str.empty()) return;

  unsigned RealOffset = getMappedOffset(OrigOffset, InsertAfter);
  Buffer.insert(RealOffset, Str.begin(), Str.end());

  // Add a delta so that future changes are offset correctly.
  AddInsertDelta(OrigOffset, Str.size());
}

/// ReplaceText - This method replaces a range of characters in the input
/// buffer with a new string.  This is effectively a combined "remove+insert"
/// operation.
void parallel::RewriteBuffer::ReplaceText(unsigned OrigOffset, unsigned OrigLength,
                                StringRef NewStr) {
  unsigned RealOffset = getMappedOffset(OrigOffset, true);
  Buffer.erase(RealOffset, OrigLength);
  Buffer.insert(RealOffset, NewStr.begin(), NewStr.end());
  if (OrigLength != NewStr.size())
    AddReplaceDelta(OrigOffset, NewStr.size() - OrigLength);
}


//===----------------------------------------------------------------------===//
// Rewriter class
//===----------------------------------------------------------------------===//

/// getRangeSize - Return the size in bytes of the specified range if they
/// are in the same file.  If not, this returns -1.
int parallel::Rewriter::getRangeSize(const CharSourceRange &Range,
                           RewriteOptions opts) const {
  if (!isRewritable(Range.getBegin()) ||
      !isRewritable(Range.getEnd())) return -1;

  FileID StartFileID, EndFileID;
  unsigned StartOff, EndOff;

  StartOff = getLocationOffsetAndFileID(Range.getBegin(), StartFileID);
  EndOff   = getLocationOffsetAndFileID(Range.getEnd(), EndFileID);

  if (StartFileID != EndFileID)
    return -1;

  // If edits have been made to this buffer, the delta between the range may
  // have changed.
  std::map<FileID, RewriteBuffer>::const_iterator I =
    RewriteBuffers.find(StartFileID);
  if (I != RewriteBuffers.end()) {
    const RewriteBuffer &RB = I->second;
    EndOff = RB.getMappedOffset(EndOff, opts.IncludeInsertsAtEndOfRange);
    StartOff = RB.getMappedOffset(StartOff, !opts.IncludeInsertsAtBeginOfRange);
  }


  // Adjust the end offset to the end of the last token, instead of being the
  // start of the last token if this is a token range.
  if (Range.isTokenRange())
    EndOff += Lexer::MeasureTokenLength(Range.getEnd(), *SourceMgr, *LangOpts);

  return EndOff-StartOff;
}

int parallel::Rewriter::getRangeSize(SourceRange Range, RewriteOptions opts) const {
  return getRangeSize(CharSourceRange::getTokenRange(Range), opts);
}


/// getRewrittenText - Return the rewritten form of the text in the specified
/// range.  If the start or end of the range was unrewritable or if they are
/// in different buffers, this returns an empty string.
///
/// Note that this method is not particularly efficient.
///
std::string parallel::Rewriter::getRewrittenText(SourceRange Range) const {
  if (!isRewritable(Range.getBegin()) ||
      !isRewritable(Range.getEnd()))
    return "";

  FileID StartFileID, EndFileID;
  unsigned StartOff, EndOff;
  StartOff = getLocationOffsetAndFileID(Range.getBegin(), StartFileID);
  EndOff   = getLocationOffsetAndFileID(Range.getEnd(), EndFileID);

  if (StartFileID != EndFileID)
    return ""; // Start and end in different buffers.

  // If edits have been made to this buffer, the delta between the range may
  // have changed.
  std::map<FileID, RewriteBuffer>::const_iterator I =
    RewriteBuffers.find(StartFileID);
  if (I == RewriteBuffers.end()) {
    // If the buffer hasn't been rewritten, just return the text from the input.
    const char *Ptr = SourceMgr->getCharacterData(Range.getBegin());

    // Adjust the end offset to the end of the last token, instead of being the
    // start of the last token.
    EndOff += Lexer::MeasureTokenLength(Range.getEnd(), *SourceMgr, *LangOpts);
    return std::string(Ptr, Ptr+EndOff-StartOff);
  }

  const RewriteBuffer &RB = I->second;
  EndOff = RB.getMappedOffset(EndOff, true);
  StartOff = RB.getMappedOffset(StartOff);

  // Adjust the end offset to the end of the last token, instead of being the
  // start of the last token.
  EndOff += Lexer::MeasureTokenLength(Range.getEnd(), *SourceMgr, *LangOpts);

  // Advance the iterators to the right spot, yay for linear time algorithms.
  RewriteBuffer::iterator Start = RB.begin();
  std::advance(Start, StartOff);
  RewriteBuffer::iterator End = Start;
  std::advance(End, EndOff-StartOff);

  return std::string(Start, End);
}

unsigned parallel::Rewriter::getLocationOffsetAndFileID(SourceLocation Loc,
                                              FileID &FID) const {
  assert(Loc.isValid() && "Invalid location");
  std::pair<FileID,unsigned> V = SourceMgr->getDecomposedLoc(Loc);
  FID = V.first;
  return V.second;
}

/// getEditBuffer - Get or create a RewriteBuffer for the specified FileID.
///
parallel::RewriteBuffer & parallel::Rewriter::getEditBuffer(FileID FID) {
  std::map<FileID, RewriteBuffer>::iterator I =
    RewriteBuffers.lower_bound(FID);
  if (I != RewriteBuffers.end() && I->first == FID)
    return I->second;
  I = RewriteBuffers.insert(I, std::make_pair(FID, RewriteBuffer()));
  StringRef MB = SourceMgr->getBufferData(FID);
  I->second.Initialize(MB.begin(), MB.end());

  return I->second;
}

/// InsertText - Insert the specified string at the specified location in the
/// original buffer.
bool parallel::Rewriter::InsertText(SourceLocation Loc, StringRef Str,
                          bool InsertAfter, bool indentNewLines) {
  if (!isRewritable(Loc)) return true;
  FileID FID;
  unsigned StartOffs = getLocationOffsetAndFileID(Loc, FID);

  SmallString<128> indentedStr;
  if (indentNewLines && Str.find('\n') != StringRef::npos) {
    StringRef MB = SourceMgr->getBufferData(FID);

    unsigned lineNo = SourceMgr->getLineNumber(FID, StartOffs) - 1;
    const SrcMgr::ContentCache *
        Content = SourceMgr->getSLocEntry(FID).getFile().getContentCache();
    unsigned lineOffs = Content->SourceLineCache[lineNo];

    // Find the whitespace at the start of the line.
    StringRef indentSpace;
    {
      unsigned i = lineOffs;
      while (isWhitespace(MB[i]))
        ++i;
      indentSpace = MB.substr(lineOffs, i-lineOffs);
    }

    SmallVector<StringRef, 4> lines;
    Str.split(lines, "\n");

    for (unsigned i = 0, e = lines.size(); i != e; ++i) {
      indentedStr += lines[i];
      if (i < e-1) {
        indentedStr += '\n';
        indentedStr += indentSpace;
      }
    }
    Str = indentedStr.str();
  }

  getEditBuffer(FID).InsertText(StartOffs, Str, InsertAfter);
  return false;
}

bool parallel::Rewriter::InsertTextAfterToken(SourceLocation Loc, StringRef Str) {
  if (!isRewritable(Loc)) return true;
  FileID FID;
  unsigned StartOffs = getLocationOffsetAndFileID(Loc, FID);
  RewriteOptions rangeOpts;
  rangeOpts.IncludeInsertsAtBeginOfRange = false;
  StartOffs += getRangeSize(SourceRange(Loc, Loc), rangeOpts);
  getEditBuffer(FID).InsertText(StartOffs, Str, /*InsertAfter*/true);
  return false;
}

/// RemoveText - Remove the specified text region.
bool parallel::Rewriter::RemoveText(SourceLocation Start, unsigned Length,
                          RewriteOptions opts) {
  if (!isRewritable(Start)) return true;
  FileID FID;
  unsigned StartOffs = getLocationOffsetAndFileID(Start, FID);
  getEditBuffer(FID).RemoveText(StartOffs, Length, opts.RemoveLineIfEmpty);
  return false;
}

/// ReplaceText - This method replaces a range of characters in the input
/// buffer with a new string.  This is effectively a combined "remove/insert"
/// operation.
bool parallel::Rewriter::ReplaceText(SourceLocation Start, unsigned OrigLength,
                           StringRef NewStr) {
  if (!isRewritable(Start)) return true;
  FileID StartFileID;
  unsigned StartOffs = getLocationOffsetAndFileID(Start, StartFileID);

  getEditBuffer(StartFileID).ReplaceText(StartOffs, OrigLength, NewStr);
  return false;
}

bool parallel::Rewriter::ReplaceText(SourceRange range, SourceRange replacementRange) {
  if (!isRewritable(range.getBegin())) return true;
  if (!isRewritable(range.getEnd())) return true;
  if (replacementRange.isInvalid()) return true;
  SourceLocation start = range.getBegin();
  unsigned origLength = getRangeSize(range);
  unsigned newLength = getRangeSize(replacementRange);
  FileID FID;
  unsigned newOffs = getLocationOffsetAndFileID(replacementRange.getBegin(),
                                                FID);
  StringRef MB = SourceMgr->getBufferData(FID);
  return ReplaceText(start, origLength, MB.substr(newOffs, newLength));
}
/// raw_String_ostream - A raw_ostream that writes to an std::string.  This is a
/// simple adaptor class. This class does not encounter output errors.
class raw_String_ostream : public raw_ostream {
  std::string &OS;

  /// write_impl - See raw_ostream::write_impl.
  //virtual void write_impl(const char *Ptr, size_t Size) LLVM_OVERRIDE;
    void write_impl(const char *Ptr, size_t Size) {
    OS.append(Ptr, Size);
  }

  /// current_pos - Return the current position within the stream, not
  /// counting the bytes currently in the buffer.
  virtual uint64_t current_pos() const LLVM_OVERRIDE { return OS.size(); }
public:
  explicit raw_String_ostream(std::string&O) : OS(O) {}
  ~raw_String_ostream() { flush(); }

  /// str - Flushes the stream contents to the target string and returns
  ///  the string's reference.
  std::string& str() {
    flush();
    return OS;
  }
};


/// ReplaceStmt - This replaces a Stmt/Expr with another, using the pretty
/// printer to generate the replacement code.  This returns true if the input
/// could not be rewritten, or false if successful.
bool parallel::Rewriter::ReplaceStmt(Stmt *From, Stmt *To) {
  // Measaure the old text.
  int Size = getRangeSize(From->getSourceRange());
  if (Size == -1)
    return true;

  // Get the new text.
  std::string SStr;
  raw_String_ostream S(SStr);
  To->printPretty(S, 0, PrintingPolicy(*LangOpts));
  const std::string &Str = S.str();

  ReplaceText(From->getLocStart(), Size, Str);
  return false;
}

StringRef parallel::Rewriter::ConvertToString(Stmt *From) {
  std::string SStr;
  raw_String_ostream S(SStr);
  From->printPretty(S, 0, PrintingPolicy(*LangOpts));
  return StringRef(S.str());
}

bool parallel::Rewriter::IncreaseIndentation(CharSourceRange range,
                                   SourceLocation parentIndent) {
  if (range.isInvalid()) return true;
  if (!isRewritable(range.getBegin())) return true;
  if (!isRewritable(range.getEnd())) return true;
  if (!isRewritable(parentIndent)) return true;

  FileID StartFileID, EndFileID, parentFileID;
  unsigned StartOff, EndOff, parentOff;

  StartOff = getLocationOffsetAndFileID(range.getBegin(), StartFileID);
  EndOff   = getLocationOffsetAndFileID(range.getEnd(), EndFileID);
  parentOff = getLocationOffsetAndFileID(parentIndent, parentFileID);

  if (StartFileID != EndFileID || StartFileID != parentFileID)
    return true;
  if (StartOff > EndOff)
    return true;

  FileID FID = StartFileID;
  StringRef MB = SourceMgr->getBufferData(FID);

  unsigned parentLineNo = SourceMgr->getLineNumber(FID, parentOff) - 1;
  unsigned startLineNo = SourceMgr->getLineNumber(FID, StartOff) - 1;
  unsigned endLineNo = SourceMgr->getLineNumber(FID, EndOff) - 1;
  
  const SrcMgr::ContentCache *
      Content = SourceMgr->getSLocEntry(FID).getFile().getContentCache();
  
  // Find where the lines start.
  unsigned parentLineOffs = Content->SourceLineCache[parentLineNo];
  unsigned startLineOffs = Content->SourceLineCache[startLineNo];

  // Find the whitespace at the start of each line.
  StringRef parentSpace, startSpace;
  {
    unsigned i = parentLineOffs;
    while (isWhitespace(MB[i]))
      ++i;
    parentSpace = MB.substr(parentLineOffs, i-parentLineOffs);

    i = startLineOffs;
    while (isWhitespace(MB[i]))
      ++i;
    startSpace = MB.substr(startLineOffs, i-startLineOffs);
  }
  if (parentSpace.size() >= startSpace.size())
    return true;
  if (!startSpace.startswith(parentSpace))
    return true;

  StringRef indent = startSpace.substr(parentSpace.size());

  // Indent the lines between start/end offsets.
  RewriteBuffer &RB = getEditBuffer(FID);
  for (unsigned lineNo = startLineNo; lineNo <= endLineNo; ++lineNo) {
    unsigned offs = Content->SourceLineCache[lineNo];
    unsigned i = offs;
    while (isWhitespace(MB[i]))
      ++i;
    StringRef origIndent = MB.substr(offs, i-offs);
    if (origIndent.startswith(startSpace))
      RB.InsertText(offs, indent, /*InsertAfter=*/false);
  }

  return false;
}

