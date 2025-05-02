// RUN: %check_clang_tidy %s misc-equalities %t

struct S {
    int i;
    int j;

    [[clang::annotate("deriving_eq")]]
    bool f_ok(const S&s) const {
        return i == s.i && j == s.j;
    }

    [[clang::annotate("deriving_eq")]]
    bool f_ok_paren(const S&s) const {
        return (i == (s.i)) && (j) == s.j;
    }

    // Body-related tests

    [[clang::annotate("deriving_eq")]]
    bool f_void(const S& s) const {}
    // CHECK-MESSAGES: :[[@LINE-1]]:35: warning: function 'f_void' has empty body but should return a boolean value
    // CHECK-MESSAGES: :[[@LINE-2]]:35: warning: function 'f_void' should consist of a single return statement composed of binary equality comparisons [misc-equalities]
    // CHECK-FIXES: bool f_void(const S& s) const  { return (i == s.i) && (j == s.j); }

    [[clang::annotate("deriving_eq")]]
    bool f_too_many_statements(const S& s) const {
        bool eq_i = i == s.i;
        bool eq_j = j == s.j;
        return eq_i && eq_j;
    }
    // CHECK-MESSAGES: :[[@LINE-5]]:50: warning: function 'f_too_many_statements' should consist of a single return statement [misc-equalities]
    // CHECK-MESSAGES: :[[@LINE-6]]:50: warning: function 'f_too_many_statements' should consist of a single return statement composed of binary equality comparisons [misc-equalities]
    // CHECK-FIXES: bool f_too_many_statements(const S& s) const  { return (i == s.i) && (j == s.j); }

        
    [[clang::annotate("deriving_eq")]]
    bool f_wrong_expression(const S& s) const {
        return i == s.i && i == s.i; // i compared twice, j missing
    }
    // CHECK-MESSAGES: :[[@LINE-3]]:47: warning: function 'f_wrong_expression' should consist of a single return statement composed of binary equality comparisons [misc-equalities]
    // CHECK-FIXES: bool f_wrong_expression(const S& s) const { return (i == s.i) && (j == s.j); }

    // Signature-related tests

    [[clang::annotate("deriving_eq")]]
    bool f_undefined_ok(const S& s) const;

    [[clang::annotate("deriving_eq")]]
    bool f_defined_later(const S& s) const;
    
    [[clang::annotate("deriving_eq")]]
    bool f_non_const_param(S& s) const;
    // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: function 'f_non_const_param' signature is not suitable for an equality operator [misc-equalities]
    // CHECK-MESSAGES: :[[@LINE-2]]:31: warning: parameter 's' should be const qualified [misc-equalities]
    // CHECK-FIXES: bool f_non_const_param(S const& s) const;
    
    [[clang::annotate("deriving_eq")]]
    bool f_non_const(const S& s);
    // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: function 'f_non_const' should be const [misc-equalities]
    // CHECK-MESSAGES: :[[@LINE-2]]:10: warning: function 'f_non_const' signature is not suitable for an equality operator [misc-equalities]
    // CHECK-FIXES: bool f_non_const(S const& s) const;
    
    [[clang::annotate("deriving_eq")]]
    bool f_non_ref(const S s) const;
    // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: function 'f_non_ref' signature is not suitable for an equality operator [misc-equalities]
    // CHECK-MESSAGES: :[[@LINE-2]]:28: warning: parameter 's' should be passed by reference [misc-equalities]
    // CHECK-FIXES: bool f_non_ref(S const& s) const;

    [[clang::annotate("deriving_eq")]]
    bool f_wrong_param_type(const int& s) const;
    // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: function 'f_wrong_param_type' has invalid argument type 'int' for equality with 'S' [misc-equalities]
    // CHECK-MESSAGES: :[[@LINE-2]]:10: warning: function 'f_wrong_param_type' signature is not suitable for an equality operator [misc-equalities]
    // CHECK-FIXES: bool f_wrong_param_type(S const& s) const;
};

struct Empty {
    [[clang::annotate("deriving_eq")]]
    bool f_trivial(Empty const& e) const {
        return false && false;
    }
    // CHECK-MESSAGES: :[[@LINE-3]]:42: warning: function 'f_trivial' should consist of a single return statement composed of binary equality comparisons
    // CHECK-FIXES: bool f_trivial(Empty const& e) const { return true; }
};


bool S::f_defined_later(const S& s) const {
    return true;
}
// CHECK-MESSAGES: :[[@LINE-3]]:43: warning: function 'f_defined_later' should consist of a single return statement composed of binary equality comparisons [misc-equalities]
// CHECK-MESSAGES: :[[@LINE-3]]:12: warning: function 'f_defined_later' returned expression should be a boolean conjonction of equalities [misc-equalities]
// CHECK-FIXES: bool S::f_defined_later(const S& s) const  { return (i == s.i) && (j == s.j); }