#include <wctype.h>

int iswctype_l(wint_t c, wctype_t t, locale_t l)
{
	return iswctype(c, t);
}
