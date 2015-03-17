// RUN: %cxxamp %s -o %t.out && %t.out
#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>

//
// It was discovered that Clang 3.3 and 3.4 would leak memory if a by-copy [=]
// variable in a lambda function is from templated classes.  For example, class
// foo down below, or Concurrency::array_view.
//
// Although this test case doesn't contain any C++AMP code, it is used to test
// if Clang contains the patch described at:
// http://lists.cs.uiuc.edu/pipermail/cfe-commits/Week-of-Mon-20140120/097658.html
//
// Prior to this patch, all C++AMP codes which use array_view in parallel_for_each
// would leak memory because array_view objects are always captured by copy.
//

class blah {
    std::shared_ptr<int> ptr;
public:
    blah() : ptr(std::make_shared<int>(0)) { }
    blah(const blah& b) : ptr(b.ptr) { }
    ~blah() { }

    void doit() const {
    }

    int count() {
        return ptr.use_count();
    }
};

template <typename T, int N>
class foo {
    std::shared_ptr<T> ptr;
public:
    foo() : ptr(std::make_shared<T>(0)) { }
    foo(const foo& f) : ptr(f.ptr) { }
    ~foo() { }

    void doit() const {
    }

    int count() {
        return ptr.use_count();
    }
};

void run_blah(blah& bbbbb) {
    std::vector<int> nums { 0, 1, 2, 3, 4 };

    // in this lambda,
    // copy constructor of bbbbb would be called twice,
    // destructor of bbbbb would also be called twice,
    // so there is no leak
    std::for_each(nums.begin(), nums.end(), [=](int& n) {
       bbbbb.doit();
    });
}

void run_foo(foo<int, 1>& fffff) {
    std::vector<int> nums { 0, 1, 2, 3, 4 };

    // in this lambda,
    // prior to the patch, Clang would incorrectly define a trivial copy 
    // constructor and call it once, and call the copy constructor once,
    // which would cause the reference count number in the shared_ptr within
    // foo instance incorrect, and cause memory leak
    //
    // after the patch, it would behave just like run_blah() above
    std::for_each(nums.begin(), nums.end(), [=](int& n) {
       fffff.doit();
    });
}

int main() {
    blah b1;
    run_blah(b1);

    foo<int, 1> f1;
    run_foo(f1);

    // make sure b1 is the last instance holding shared_ptr,
    // i.e. there was no memory leaked in run_blah()
    assert(b1.count() == 1); 

    // make sure f1 is the last instance holding shared_ptr
    // i.e. there was no memory leaked in run_foo()
    assert(f1.count() == 1);

    return 0;
}
