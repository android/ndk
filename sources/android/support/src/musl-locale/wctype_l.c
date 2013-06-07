#include <wctype.h>

wctype_t wctype_l(const char *s, locale_t l)
{
	return wctype(s);
}
