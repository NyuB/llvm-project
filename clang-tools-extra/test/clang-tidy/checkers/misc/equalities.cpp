// RUN: %check_clang_tidy %s misc-equalities %t

struct S {
    int i;
    int j;

    [[clang::annotate("deriving_eq")]]
    bool f_undefined(const S& s) const;
    
    /// @brief 
    /// @param s 
    [[clang::annotate("deriving_eq")]]
    void f_void(const S& s) const {}
    // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: function 'f_void' has empty body but should return a boolean value [misc-equalities]
    // CHECK-MESSAGES: :[[@LINE-2]]:10: warning: function 'f_void' has non boolean return type [misc-equalities]
    // CHECK-MESSAGES: :[[@LINE-3]]:10: warning: function 'f_void' signature is not suitable for an equality operator [misc-equalities]
    // CHECK-MESSAGES: :[[@LINE-4]]:35: warning: function 'f_void' should consist of a single return statement composed of binary equality comparisons [misc-equalities]
    // CHECK-FIXES: { return (i == s.i) && (j == s.j); }
    [[clang::annotate("deriving_eq")]]
    bool f_too_many_statements(const S& s) const {
        bool r = true;
        return r;
    }

    [[clang::annotate("deriving_eq")]]
    bool f_no_return(const S& s) const {
        bool r = true;
    }

    [[clang::annotate("deriving_eq")]]
    bool f_no_single_param() const {
        bool r = true;
    }

    [[clang::annotate("deriving_eq")]]
    bool f_non_const_param(S &s) const {
        return true;
    }

    [[clang::annotate("deriving_eq")]]
    bool f_non_ref_param(const S s) const {
        return true;
    }

    [[clang::annotate("deriving_eq")]]
    bool f_non_const(const S& s) {
        return true;
    }

    [[clang::annotate("deriving_eq")]]
    bool f_ok(const S& s) const {
        return (true == true) && (false == false);
    }
};

void awesome_f2();
