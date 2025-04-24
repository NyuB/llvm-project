// RUN: %check_clang_tidy %s misc-hello %t

[[clang::annotate("Hello")]]
void f();
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'f' is annotated with Hello and should therefore be prefixed with 'hello_' [misc-hello]

// * Make the CHECK patterns specific enough and try to make verified lines
//     unique to avoid incorrect matches.
// * Use {{}} for regular expressions.
// CHECK-FIXES: {{^}}void hello_f();{{$}}

[[clang::annotate("Hello")]]
void hello_f2();
