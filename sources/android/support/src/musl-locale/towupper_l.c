#include <wctype.h>

wint_t towupper_l(wint_t c, locale_t l)
{
	return towupper(c);
}
