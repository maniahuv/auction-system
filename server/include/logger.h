#ifndef LOGGER_H
#define LOGGER_H

// Ghi log hoat dong cua nguoi dung vao database va file
// username: ten nguoi dung
// action: mo ta hanh dong da thuc hien
void log_activity(const char *username, const char *action);

#endif