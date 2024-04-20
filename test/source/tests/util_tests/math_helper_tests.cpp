#include "concurrencpp/utils/math_helper.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include <iostream>

namespace concurrencpp::tests {
    void test_math_helper_is_prime();
    void test_math_helper_next_prime();

    constexpr size_t prime_numbers[] = {2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,  61,  67,
                                        71,  73,  79,  83,  89,  97,  101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163,
                                        167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269,
                                        271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383,
                                        389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499};

    constexpr size_t non_prime_numbers[] = {
        4,   6,   8,   9,   10,  12,  14,  15,  16,  18,  20,  21,  22,  24,  25,  26,  27,  28,  30,  32,  33,
        34,  35,  36,  38,  39,  40,  42,  44,  45,  46,  48,  49,  50,
        51,  52,  54,  55,  56,  57,  58,  60,  62,  63,  64,  65,  66,  68,  69,  70,  72,  74,  75,  76,  77,
        78,  80,  81,  82,  84,  85,  86,  87,  88,  90,  91,  92,  93,  94,  95,  96,  98,  99,  100,
        102, 104, 105, 106, 108, 110, 111, 112, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
        128, 129, 130, 132, 133, 134, 135, 136, 138, 140, 141, 142, 143, 144, 145, 146, 147, 148, 150,
        152, 153, 154, 155, 156, 158, 159, 160, 161, 162, 164, 165, 166, 168, 169, 170, 171, 172, 174, 175, 176,
        177, 178, 180, 182, 183, 184, 185, 186, 187, 188, 189, 190, 192, 194, 195, 196, 198, 200,
        201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222,
        224, 225, 226, 228, 230, 231, 232, 234, 235, 236, 237, 238, 240, 242, 243, 244, 245, 246, 247, 248, 249,
        250, 252, 253, 254, 255, 256, 258, 259, 260, 261, 262, 264, 265, 266, 267, 268, 270, 272, 273, 274, 275,
        276, 278, 279, 280, 282, 284, 285, 286, 287, 288, 289, 290, 291, 292, 294, 295, 296, 297, 298, 299};
}  // namespace concurrencpp::tests

using namespace concurrencpp::tests;
using concurrencpp::details::math_helper;


void concurrencpp::tests::test_math_helper_is_prime() {
    for (const auto i : prime_numbers) {
        assert_true(math_helper::is_prime(i));
    }

    for (const auto i : non_prime_numbers) {
        assert_false(math_helper::is_prime(i));
    }
}

void concurrencpp::tests::test_math_helper_next_prime() {
    size_t last_prime_number = 0;
    
    for (const auto prime_number : prime_numbers) {
        for (auto start = last_prime_number + 1; start < prime_number; start++) {
            const auto next_prime = math_helper::next_prime(start);
            assert_equal(next_prime, prime_number);
        }
    
        last_prime_number = prime_number;
    }
}

int main() {
    tester tester("math_helper test");

    tester.add_step("is_prime", test_math_helper_is_prime);
    tester.add_step("next_prime", test_math_helper_next_prime);

    tester.launch_test();
    return 0;
}
