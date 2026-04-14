// RUN: %check_clang_tidy %s nyub-deriving-eq %t -- --extra-arg=-fno-delayed-template-parsing

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
    // CHECK-MESSAGES: :[[@LINE-2]]:35: warning: function 'f_void' should consist of a single return statement composed of binary equality comparisons [nyub-deriving-eq]
    // CHECK-FIXES: bool f_void(const S& s) const  { return (i == s.i) && (j == s.j); }

    [[clang::annotate("deriving_eq")]]
    bool f_too_many_statements(const S& s) const {
        bool eq_i = i == s.i;
        bool eq_j = j == s.j;
        return eq_i && eq_j;
    }
    // CHECK-MESSAGES: :[[@LINE-5]]:50: warning: function 'f_too_many_statements' should consist of a single return statement [nyub-deriving-eq]
    // CHECK-MESSAGES: :[[@LINE-6]]:50: warning: function 'f_too_many_statements' should consist of a single return statement composed of binary equality comparisons [nyub-deriving-eq]
    // CHECK-FIXES: bool f_too_many_statements(const S& s) const  { return (i == s.i) && (j == s.j); }

        
    [[clang::annotate("deriving_eq")]]
    bool f_wrong_expression(const S& s) const {
        return i == s.i && i == s.i; // i compared twice, j missing
    }
    // CHECK-MESSAGES: :[[@LINE-3]]:47: warning: function 'f_wrong_expression' should consist of a single return statement composed of binary equality comparisons [nyub-deriving-eq]
    // CHECK-FIXES: bool f_wrong_expression(const S& s) const { return (i == s.i) && (j == s.j); }

    // Signature-related tests

    [[clang::annotate("deriving_eq")]]
    bool f_undefined_ok(const S& s) const;

    [[clang::annotate("deriving_eq")]]
    bool f_defined_later(const S& s) const;
    
    [[clang::annotate("deriving_eq")]]
    bool f_non_const_param(S& s) const;
    // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: function 'f_non_const_param' signature is not suitable for an equality operator [nyub-deriving-eq]
    // CHECK-MESSAGES: :[[@LINE-2]]:31: warning: parameter 's' should be const qualified [nyub-deriving-eq]
    // CHECK-FIXES: bool f_non_const_param(S const& s) const;
    
    [[clang::annotate("deriving_eq")]]
    bool f_non_const(const S& s);
    // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: function 'f_non_const' should be const [nyub-deriving-eq]
    // CHECK-MESSAGES: :[[@LINE-2]]:10: warning: function 'f_non_const' signature is not suitable for an equality operator [nyub-deriving-eq]
    // CHECK-FIXES: bool f_non_const(S const& s) const;
    
    [[clang::annotate("deriving_eq")]]
    bool f_non_ref(const S s) const;
    // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: function 'f_non_ref' signature is not suitable for an equality operator [nyub-deriving-eq]
    // CHECK-MESSAGES: :[[@LINE-2]]:28: warning: parameter 's' should be passed by reference [nyub-deriving-eq]
    // CHECK-FIXES: bool f_non_ref(S const& s) const;

    [[clang::annotate("deriving_eq")]]
    bool f_wrong_param_type(const int& s) const;
    // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: function 'f_wrong_param_type' has invalid argument type 'int' for equality with 'S' [nyub-deriving-eq]
    // CHECK-MESSAGES: :[[@LINE-2]]:10: warning: function 'f_wrong_param_type' signature is not suitable for an equality operator [nyub-deriving-eq]
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

struct Ignored {
    int n;
    [[clang::annotate("deriving_eq::ignore")]]
    int ignored;
    
    [[clang::annotate("deriving_eq")]]
    bool f_ok(Ignored const& i) const {
        return n == i.n;
    }
};


bool S::f_defined_later(const S& s) const {
    return true;
}
// CHECK-MESSAGES: :[[@LINE-3]]:43: warning: function 'f_defined_later' should consist of a single return statement composed of binary equality comparisons [nyub-deriving-eq]
// CHECK-MESSAGES: :[[@LINE-3]]:12: warning: function 'f_defined_later' returned expression should be a boolean conjonction of equalities [nyub-deriving-eq]
// CHECK-FIXES: bool S::f_defined_later(const S& s) const  { return (i == s.i) && (j == s.j); }



struct OperatorCall {
    struct str {
        bool operator==(str const& other) const { return true; }
    };
    str s;

    [[clang::annotate("deriving_eq")]]
    bool operator==(const OperatorCall &other) const { 
        return s == other.s;
    }
};

template<typename T>
struct Templated {
    T t;

    [[clang::annotate("deriving_eq")]]
    bool f_ok(Templated const& other) const {
        return (t == other.t);
    }

    [[clang::annotate("deriving_eq")]]
    bool f_void(Templated const& other) const {}
    // CHECK-MESSAGES: :[[@LINE-1]]:47: warning: function 'f_void' has empty body but should return a boolean value
    // CHECK-MESSAGES: :[[@LINE-2]]:47: warning: function 'f_void' should consist of a single return statement composed of binary equality comparisons [nyub-deriving-eq]
    // CHECK-FIXES: bool f_void(Templated const& other) const { return (t == other.t); }
};

struct NonRegression_ImplicitIntCastToInt {
    char c;

    [[clang::annotate("deriving_eq")]]
    bool operator==(const NonRegression_ImplicitIntCastToInt &other) const { 
        return c == other.c;
    }
};

struct NonRegression_PreserveAnnotations {
    [[clang::annotate("deriving_eq")]] bool f_decl(NonRegression_PreserveAnnotations const& other);
    // CHECK-MESSAGES: :[[@LINE-1]]:45: warning: function 'f_decl' should be const [nyub-deriving-eq]
    // CHECK-MESSAGES: :[[@LINE-2]]:45: warning: function 'f_decl' signature is not suitable for an equality operator [nyub-deriving-eq]  
    // Double brackets are interpreted as test-specific annotations so we must escape them as regexes in the check-fixes instruction
    // CHECK-FIXES: {{\[\[}}clang::annotate("deriving_eq"){{\]\]}} bool f_decl(NonRegression_PreserveAnnotations const& other) const;

    [[clang::annotate("deriving_eq")]] auto f_decl_auto(NonRegression_PreserveAnnotations const& other) -> bool;
    // CHECK-MESSAGES: :[[@LINE-1]]:45: warning: function 'f_decl_auto' should be const [nyub-deriving-eq]
    // CHECK-MESSAGES: :[[@LINE-2]]:45: warning: function 'f_decl_auto' signature is not suitable for an equality operator [nyub-deriving-eq]  
    // Double brackets are interpreted as test-specific annotations so we must escape them as regexes in the check-fixes instruction
    // CHECK-FIXES: {{\[\[}}clang::annotate("deriving_eq"){{\]\]}} bool f_decl_auto(NonRegression_PreserveAnnotations const& other) const;
};
