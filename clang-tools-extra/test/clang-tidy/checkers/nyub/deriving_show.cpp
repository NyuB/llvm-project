// RUN: %check_clang_tidy %s nyub-deriving-show %t

#include <iosfwd>
struct S {
    int i;
    int j;

    // Signature-related checks
    [[clang::annotate("deriving_show")]]
    static std::ostream& f_ok(std::ostream& os, S const& s);

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
};
