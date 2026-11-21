#include <cstdio>

template <typename T>
class MyVector {
public:
    MyVector();
    ~MyVector();
    MyVector(const MyVector& other);              // 拷贝构造
    MyVector& operator=(const MyVector& other);   // 拷贝赋值
    void push_back(const T& v);             // 含 2 倍扩容
    void reserve(size_t n);
    T& operator[](size_t i);
    size_t size() const;
    size_t capacity() const;
private:
    T* data_;
    size_t size_;
    size_t capacity_;
};

template <typename T>
MyVector<T>::MyVector() {
    data_ = nullptr;
    size_ = 0;
    capacity_ = 0;
}

template <typename T>
MyVector<T>::~MyVector() {
    delete[] data_;
}

template <typename T>
MyVector<T>::MyVector(const MyVector& other) {
    data_ = new T[other.capacity_];
    for (size_t i = 0;i < other.size_;i++) {
        data_[i] = other.data_[i];
    }
    size_ = other.size_;
    capacity_ = other.capacity_;
}

template <typename T>
MyVector<T>& MyVector<T>::operator=(const MyVector& other) {
    if (this == &other) return *this;
    delete[] data_;
    data_ = new T[other.capacity_];
    for (size_t i = 0;i < other.size_;i++) {
        data_[i] = other.data_[i];
    }
    size_ = other.size_;
    capacity_ = other.capacity_;
    return *this;
}

template <typename T>
size_t MyVector<T>::size() const {
    return size_;
}

template <typename T>
size_t MyVector<T>::capacity() const {
    return capacity_;
}

template <typename T>
void MyVector<T>::reserve(size_t n) {
    if (n <= capacity_) return;
    T* data1 = new T[n];
    for (size_t i = 0;i < size_;i++) {
        data1[i] = data_[i];
    }
    delete[] data_;
    data_ = data1;
    capacity_ = n;
}

template <typename T>
void MyVector<T>::push_back(const T& v) {
    if (size_ + 1 > capacity_) {
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }
    data_[size_] = v;
    size_ = size_ + 1;
}

template <typename T>
T& MyVector<T>::operator[](size_t i) {
    return data_[i];
}