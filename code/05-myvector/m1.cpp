#include <cstdio>
#include <utility>
#include <cstddef>

template <typename T>
class MyVector {
    public:
        MyVector();
        ~MyVector();
        size_t _size() const;
        size_t _capacity() const;
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
        size_t capacity_;
};

template <typename T>
MyVector<T>::MyVector() {
    data = nullptr;
    size_ = 0;
    capacity_ = 0;
}

template <typename T>
MyVector<T>::~MyVector() {
    delete[] data;
}

template <typename T>
size_t MyVector<T>::_size() const{
    return size_;
}

template <typename T>
size_t MyVector<T>::_capacity() const {
    return capacity_;
}

template <typename T>
T& MyVector<T>::operator[](size_t i) {
    return data[i];
}

template <typename T>
void MyVector<T>::reserve(size_t n) {
    if (n <= capacity_) return;
    T *data1 = new T[n];
    for (size_t i = 0;i < size_;i++) {
        data1[i] = std::move(data[i]);
    }
    delete[] data;
    data = data1;
    capacity_ = n;
    return;
}

template <typename T>
void MyVector<T>::push_back(const T& v) {
    if (size_ + 1 > capacity_) {
        if (capacity_ != 0) {
            reserve (capacity_ * 2);
        } else {
            reserve (capacity_ + 1);
        }
    }
    data[size_] = v;
    size_++;
}

template <typename T>
void MyVector<T>::push_back(T&& v) {
    if (size_ + 1 > capacity_) {
        if (capacity_ != 0) {
            reserve (capacity_ * 2);
        } else {
            reserve (capacity_ + 1);
        }
    }
    data[size_] = std::move(v);
    size_++;
}

template <typename T>
MyVector<T>::MyVector(const MyVector& o) {
    size_ = o.size_;
    capacity_ = o.capacity_;

    data = new T[capacity_];
    for (size_t i = 0;i < size_;i++) {
        data[i] = o.data[i];
    }
}

template <typename T>
MyVector<T>::MyVector(MyVector&& o) noexcept {
    data = o.data;
    size_ = o.size_;
    capacity_ = o.capacity_;
    o.data = nullptr;
    o.size_ = 0;
    o.capacity_ = 0;
}

template <typename T>
MyVector<T>& MyVector<T>::operator=(const MyVector& o) {
    if (this == &o) return *this;
    delete[] data;
    size_ = o.size_;
    capacity_ = o.capacity_;
    data = new T[capacity_];
    for (size_t i = 0;i < size_;i++) {
        data[i] = o.data[i];
    }
    return *this;
}

template <typename T>
MyVector<T>& MyVector<T>::operator=(MyVector&& o) noexcept {
    if (this == &o) return *this;
    delete[] data;
    size_ = o.size_;
    capacity_ = o.capacity_;
    data = o.data;
    o.data = nullptr;
    o.size_ = 0;
    o.capacity_ = 0;
    return *this;
}