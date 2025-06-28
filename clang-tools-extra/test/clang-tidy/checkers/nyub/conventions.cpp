// RUN: %check_clang_tidy %s readability-identifier-naming %t --config-file=%s.configFile

int function_ok(int p_param) {
    int l_local = p_param + 12;
    return p_param * l_local;
}

class Class_ok {
    int method_ok(int p_param) {
        return p_param * m_member;
    }
    int m_member;
};

int function_ko(int param) {
// CHECK-FIXES: int function_ko(int p_param) {
    int local = param + 2;
    // CHECK_FIXES: int l_local = p_param + 2;
    return param * local;
    // CHECK-FIXES: return p_param * l_local;
}

class Class_ko {
    int method_ko(int param) {
    // CHECK-FIXES: int method_ko(int p_param) {
        int local = param + 2;
        // CHECK_FIXES: int l_local = p_param + 2;
        return param * member * local;
        // CHECK-FIXES: return p_param * m_member * l_local;
    }
    int member;
    // CHECK-FIXES: int m_member;
};