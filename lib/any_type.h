#pragma once

#include <memory>

template<typename T>
struct remove_reference {
    using type = T;
};

template<typename T>
struct remove_reference<T&> {
    using type = T;
};

template<typename T>
struct remove_reference<T&&> {
    using type = T;
};

template<typename T>
struct remove_const {
    using type = T;
};

template<typename T>
struct remove_const<const T> {
    using type = T;
};

template<typename T>
class decay {
private:
    using type1 = typename remove_reference<T>::type;
public:
    using type = typename remove_const<type1>::type;
};

template<typename T>
using my_decay_t = typename decay<T>::type;

template<typename T>
size_t type_id() {
    static char id = 0;
    return reinterpret_cast<size_t>(&id);
}; 

class AnyType {
    struct BaseHolder {
        virtual ~BaseHolder() = default;
        virtual std::unique_ptr<BaseHolder> clone() = 0;
        virtual size_t type() const = 0;
    };

    template<typename T>
    struct Holder : public BaseHolder {

        Holder(const T& val) : val_(val) {}
        Holder(T&& val) : val_(std::move(val)) {}

        std::unique_ptr<BaseHolder> clone() override {
            return std::make_unique<Holder<T>>(val_);
        } 

        size_t type() const override {
            return type_id<T>();
        }

        T val_;
    };

    std::unique_ptr<BaseHolder> holder_ = nullptr;
public:
    AnyType() = default;

    template<typename T>
    AnyType(T&& val) 
        : holder_(std::make_unique<Holder<my_decay_t<T>>>(std::forward<T>(val))) {}


    AnyType(const AnyType& other) 
        : holder_(other.holder_ ? other.holder_->clone() : nullptr) {} 
        
    AnyType& operator=(const AnyType& other) {
        if (this != &other) {
            holder_ = other.holder_ ? other.holder_->clone() : nullptr;
        }
        return *this;
    }

    AnyType(AnyType&& other) noexcept = default;

    AnyType& operator=(AnyType&& other) noexcept = default;

    bool has_value() const noexcept {
        return holder_ != nullptr;
    }    

    size_t type() const {
        if (!holder_) {
            throw "Accessing type of empty AnyType";
        }
        return holder_->type();
    }

    AnyType clone() const {
        if (!holder_) {
            throw "Accessing clone of empty AnyType";
        }

        AnyType cloned = *this;
        return cloned;
    }

    template<typename T>
    friend T any_cast(const AnyType& any);
};

template<typename T>
T any_cast(const AnyType& any) {
    if(!any.has_value() || type_id<my_decay_t<T>>() != any.type()) {
        throw "bad cast";
    }

    auto* ptr = dynamic_cast<AnyType::Holder<my_decay_t<T>>*>(any.holder_.get());
    return ptr->val_;
}
