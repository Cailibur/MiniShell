#ifndef COLOR_H
#define COLOR_H

// ANSI escape codes for text colors and styles

#define COLOR_RESET   "\033[0m"

#define COLOR_BLACK   "\033[30m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

#define COLOR_BBLACK   "\033[90m"
#define COLOR_BRED     "\033[91m"
#define COLOR_BGREEN   "\033[92m"
#define COLOR_BYELLOW  "\033[93m"
#define COLOR_BBLUE    "\033[94m"
#define COLOR_BMAGENTA "\033[95m"
#define COLOR_BCYAN    "\033[96m"
#define COLOR_BWHITE   "\033[97m"

#define STYLE_BOLD      "\033[1m"
#define STYLE_UNDERLINE "\033[4m"
#define STYLE_RESET "\033[0m"

#define CLEAR "\033[2J\033[H"


#endif