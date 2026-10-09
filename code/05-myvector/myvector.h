#pragma once
#include <cstddef>
#include <utility>
template <typename T>
class MyVector{
    public :
        MyVector();
        ~MyVector();
        size_t size() const;
        size_t capacity() const;
        T& operator[](size_t i);
        void reserve(size_t n);
        void push_back(const T& v);
        void push_back(T&& v);
        MyVector(const MyVector& o);
        MyVector(MyVector&& o) noexcept;
        MyVector& operator=(const MyVector& o);
        MyVector& operator=(MyVector&& o) noexcept;

    private:
        T *data;
        size_t size_;
        size_t cap_;
};

template <typename T>
MyVector<T>::MyVector() {
    data = nullptr;
    size_ = 0;
    cap_ = 0;
}

template <typename T>
T& MyVector<T>::operator[](size_t i) {
    return data[i];
}

template <typename T>
MyVector<T>::~MyVector() {
    delete[] data;
    size_ = 0;
    cap_ = 0;
}

template <typename T>
size_t MyVector<T>::size() const{
    return size_;
}

template <typename T>
size_t MyVector<T>::capacity() const {
    return cap_;
}

template <typename T>
void MyVector<T>::reserve(size_t n) {
    if (n <= cap_) return;
    T* data1 = new T[n];
    for(int i = 0;i < size_;i++) {
        data1[i] = std::move(data[i]);
    }
    delete[] data;
    data = data1;
    //delete[] data1;
    cap_ = n;
}

template <typename T>
void MyVector<T>::push_back(const T& v) {
    if (size_ + 1 > cap_) {
        if (cap_ != 0)
            reserve(cap_ * 2);
        else 
            reserve(size_ + 1);
    }
    data[size_] = v;
    size_++;
    
    printf("[push_back] const T&\n");    // 左值版那个函数里
}

template <typename T>
void MyVector<T>::push_back(T&& v) {
    if (size_ + 1 > cap_) {
        if (cap_ != 0)
            reserve(cap_ * 2);
        else 
            reserve(size_ + 1);
    }
    data[size_] = std::move(v);
    size_++;
    printf("[push_back] T&&\n");         // 右值版那个函数里

}

template <typename T>
MyVector<T>::MyVector(const MyVector& o) {
    
}

template <typename T>
MyVector<T>::MyVector(MyVector&& o) noexcept {

}

template <typename T>
MyVector<T>& MyVector<T>::operator=(const MyVector& o) {

}

template <typename T>
MyVector<T>& MyVector<T>::operator=(MyVector&& o) noexcept {

}