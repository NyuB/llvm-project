// RUN: %check_clang_tidy %s nyub-deriving-show %t

#include <iosfwd>
struct S {
    int i;
    int j;

    [[clang::annotate("deriving_show")]]
    static std::ostream& f_ok(std::ostream& os, S const& s);

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
};


std::ostream & S::f_ok_defined_later(std::ostream & os, S const & s)
{
    return os;
}

void S::f_ko_defined_later(std::ostream& os, S const& s) { }
// CHECK-MESSAGES: [[@LINE-1]]:9: warning: function 'f_ko_defined_later' should return std::ostream instead of void [nyub-deriving-show]
// CHECK-MESSAGES: [[@LINE-2]]:9: warning: function 'f_ko_defined_later' signature is not suitable for string display [nyub-deriving-show]
// CHECK-FIXES: std::ostream& S::f_ko_defined_later(std::ostream& os, S const& s) { }

[[clang::annotate("deriving_show")]]
std::ostream & f_friend_defined_later(std::ostream & os, S const & s)
{
    return os;
}
