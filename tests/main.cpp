#include "concurrencpp.h"
#include "tests/all_tests.h"

int main() {
	concurrencpp::tests::test_all();

	std::getchar();
	return 0;
}