#include <iostream>

using namespace std;

int main() {

    int suma(int p, int q) {

        return p + q;
    }

    int r = suma(1, 2);

    do {
        r--;
    }

    while ( r > 0);

    switch (r) {

        case 0:
            cout << "cero";
            break;

        default:
            r = 1;
    }
}
