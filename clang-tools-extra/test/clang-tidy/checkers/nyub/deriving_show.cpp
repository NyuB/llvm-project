// RUN: %check_clang_tidy %s nyub-deriving-show %t

// To avoid having to include the entire ostream headers, just declare a no-op operator<< for ostream
namespace std {
    class ostream {};
}

template<typename T>
std::ostream& operator<<(std::ostream &os, T) {
    return os;
}

struct S {
    int i;
    int j;

    [[clang::annotate("deriving_show")]]
    static std::ostream& f_ok(std::ostream& os, S const& s) {
        return os << "{ " << ".i = " << s.i << ", .j = " << s.j << " }";
    }

    [[clang::annotate("deriving_show")]]
    static std::ostream& f_ok_defined_later(std::ostream& os, S const& s);

    // Cannot annotate friend function declaration, only definition
    friend std::ostream& f_friend_defined_later(std::ostream& os, S const& s);

    // Signature-related checks

    [[clang::annotate("deriving_show")]]
    static void f_ko_defined_later(std::ostream& os, S const& s);
    // CHECK-MESSAGES: [[@LINE-1]]:17: warning: function 'f_ko_defined_later' should return std::ostream instead of void [nyub-deriving-show]
    // CHECK-MESSAGES: [[@LINE-2]]:17: warning: function 'f_ko_defined_later' signature is not suitable for string display [nyub-deriving-show]
    // CHECK-FIXES: static std::ostream& f_ko_defined_later(std::ostream& os, S const& s);

    [[clang::annotate("deriving_show")]]
    static std::ostream& f_missing_params();
    // CHECK-MESSAGES: [[@LINE-1]]:26: warning: function 'f_missing_params' should take 2 parameters [nyub-deriving-show]
    // CHECK-MESSAGES: [[@LINE-2]]:26: warning: function 'f_missing_params' signature is not suitable for string display [nyub-deriving-show]
    // CHECK-FIXES: static std::ostream& f_missing_params(std::ostream& os, S const& s);

    [[clang::annotate("deriving_show")]]
    static std::ostream& f_first_not_ostream(int& os, S const& s);
    // CHECK-MESSAGES: [[@LINE-1]]:26: warning: function 'f_first_not_ostream' signature is not suitable for string display [nyub-deriving-show]
    // CHECK-MESSAGES: [[@LINE-2]]:51: warning: parameter 'os' should be of type std::ostream but is 'int' [nyub-deriving-show]
    // CHECK-FIXES: static std::ostream& f_first_not_ostream(std::ostream& os, S const& s);

    [[clang::annotate("deriving_show")]]
    std::ostream& f_no_static(std::ostream& os, S const& s);
    // CHECK-MESSAGES: [[@LINE-1]]:19: warning: function 'f_no_static' should be static [nyub-deriving-show]
    // CHECK-MESSAGES: [[@LINE-2]]:19: warning: function 'f_no_static' signature is not suitable for string display [nyub-deriving-show]
    // CHECK-FIXES: static std::ostream& f_no_static(std::ostream& os, S const& s);

    // Body-related checks
    [[clang::annotate("deriving_show")]]
    static std::ostream& f_missing_body(std::ostream& os, S const& s);
};


std::ostream & S::f_ok_defined_later(std::ostream & os, S const & s)
{
    return os << "{ " << ".i = " << s.i << ", .j = " << s.j << " }";
}

void S::f_ko_defined_later(std::ostream& os, S const& s) { return; }
// CHECK-MESSAGES: [[@LINE-1]]:9: warning: function 'f_ko_defined_later' should return std::ostream instead of void [nyub-deriving-show]
// CHECK-MESSAGES: [[@LINE-2]]:9: warning: function 'f_ko_defined_later' signature is not suitable for string display [nyub-deriving-show]
// CHECK-MESSAGES: [[@LINE-3]]:58: warning: function 'f_ko_defined_later' body is not suitable for string display [nyub-deriving-show]
// CHECK-MESSAGES: [[@LINE-4]]:58: warning: function 'f_ko_defined_later' body should consist of a single return statement chaining << operators [nyub-deriving-show]
// CHECK-FIXES: std::ostream& S::f_ko_defined_later(std::ostream& os, S const& s) { return os << "{ .i = " << s.i << ", .j = " << s.j << " }"; }

[[clang::annotate("deriving_show")]]
std::ostream & f_friend_defined_later(std::ostream & os, S const & s)
{
    return os << "{ " << ".i = " << s.i << ", .j = " << s.j << " }";
}

std::ostream& S::f_missing_body(std::ostream& os, S const& s) { }
// CHECK-MESSAGES: [[@LINE-1]]:63: warning: function 'f_missing_body' body is not suitable for string display [nyub-deriving-show]
// CHECK-MESSAGES: [[@LINE-2]]:63: warning: function 'f_missing_body' body should consist of a single return statement [nyub-deriving-show]
// CHECK-FIXES: std::ostream& S::f_missing_body(std::ostream& os, S const& s) { return os << "{ .i = " << s.i << ", .j = " << s.j << " }"; }

struct Qustom {
    const char* toStr() const {
        return "Ok";
    }
};

struct Transformed {
    int i;
    [[clang::annotate("deriving_show::suffix.toStr()")]]
    Qustom q;
    
    [[clang::annotate("deriving_show::prefix\"[\" << ")]]
    [[clang::annotate("deriving_show::suffix << \"]\"")]]
    bool b;

    [[clang::annotate("deriving_show")]]
    static std::ostream& f_missing_body(std::ostream& os, Transformed const& t) { }
    // CHECK-MESSAGES: [[@LINE-1]]:81: warning: function 'f_missing_body' body is not suitable for string display [nyub-deriving-show]
    // CHECK-MESSAGES: [[@LINE-2]]:81: warning: function 'f_missing_body' body should consist of a single return statement [nyub-deriving-show]
    // CHECK-FIXES: static std::ostream& f_missing_body(std::ostream& os, Transformed const& t) { return os << "{ .i = " << t.i << ", .q = " << t.q.toStr() << ", .b = " << "[" << t.b << "]" << " }"; }
};

struct NonRegression_PreserveAnnotations {
    [[clang::annotate("deriving_show")]] static std::ostream& f_decl();
    // CHECK-MESSAGES: [[@LINE-1]]:63: warning: function 'f_decl' should take 2 parameters [nyub-deriving-show]
    // CHECK-MESSAGES: [[@LINE-2]]:63: warning: function 'f_decl' signature is not suitable for string display [nyub-deriving-show]
    // Double brackets are interpreted as test-specific annotations so we must escape them as regexes in the check-fixes instruction
    // CHECK-FIXES: {{\[\[}}clang::annotate("deriving_show"){{\]\]}} static std::ostream& f_decl(std::ostream& os, NonRegression_PreserveAnnotations const& nonRegression_PreserveAnnotations);
};