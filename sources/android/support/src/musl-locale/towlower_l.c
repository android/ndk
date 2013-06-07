#include <wctype.h>

wint_t towlower_l(wint_t c, locale_t l)
{
	return towlower(c);
}
