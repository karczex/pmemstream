#include "common/util.h"
#include "unittest.hpp"
#include <rapidcheck.h>

int main(int argc, char *argv[])
{
	return run_test([] {
		return_check ret;

		ret += rc::check("ALIGN_UP Identity", [](const size_t &x) { UT_ASSERTeq(ALIGN_UP(x, x), x); });

		ret += rc::check("ALIGN_DOWN Identity", [](const size_t &x) { UT_ASSERTeq(ALIGN_DOWN(x, x), x); });

		ret += rc::check("ALIGN_UP alignment property",
				 [](const size_t &x, const size_t &y) { UT_ASSERTeq(ALIGN_UP(x, y) % y, 0); });

		ret += rc::check("ALIGN_DOWN alignment property",
				 [](const size_t &x, const size_t &y) { UT_ASSERTeq(ALIGN_DOWN(x, y) % y, 0); });

		ret += rc::check("Alingments inequality property", [](const size_t &x, const size_t &y) {
			RC_PRE(x != y);
			UT_ASSERT(ALIGN_DOWN(x, y) < ALIGN_UP(x, y));
		});

		ret += rc::check("Alingments equality property",
				 [](const size_t &x) { UT_ASSERTeq(ALIGN_DOWN(x, x), ALIGN_UP(x, x)); });
	});
}
