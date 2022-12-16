#include <iostream>
#include <math.h>
using namespace std;

string pretty_size(long long bytes) {
    if(bytes >= pow(1000, 3)) {
        return to_string((int)(bytes / pow(1000, 3))) + "GB";
    }
    else if(bytes >= pow(1000, 2)) {
        return to_string((int)(bytes / pow(1000, 2))) + "MB";
    }
    else if(bytes >= pow(1000, 1)) {
        return to_string((int)(bytes / pow(1000, 1))) + "KB";
    }
    else {
        return to_string((int)(bytes)) + "B";
    }
}