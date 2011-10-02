#include <flux.h>
/* All code is licensed under GNU General Public License GPL v3 (http://www.gnu.org/licenses/gpl.html) */
/**
 *\file  Sepstream.cpp 
 *\brief Contains the Seperator Stream class.
 */
sepstream::sepstream(const Flux::string &source, char seperator) : tokens(source), sep(seperator)
{
		last_starting_position = n = tokens.begin();
}

bool sepstream::GetToken(Flux::string &token)
{
		Flux::string::iterator lsp = last_starting_position;
		while (n != tokens.end())
		{
			if (*n == sep || n + 1 == tokens.end())
			{
				last_starting_position = n + 1;
				token = Flux::string(lsp, n + 1 == tokens.end() ? n + 1 : n);
				
				while (token.length() && token.rfind(sep) == token.length() - 1)
                                 token.erase(token.end() - 1);

				++n;
				
				return true;
			}
			++n;
		}
		token.clear();
		return false;
}
const Flux::string sepstream::GetRemaining()
{
		return Flux::string(n, tokens.end());
}
bool sepstream::StreamEnd()
{
		return n == tokens.end();
}
/******************************************************************************/

/* The following code was ported from anope, all credits goes to the Anope Team
 *
 * This is an implementation of two special string classes:
 *
 * irc::string which is a case-insensitive equivalent to
 * std::string which is not only case-insensitive but
 * can also do scandanavian comparisons, e.g. { = [, etc.
 *
 * ci::string which is a case-insensitive equivalent to
 * std::string.
 *
 * These classes depend on ascii_case_insensitive_map
 *
 */
 
/* VS 2008 specific function */
bool Flux::hash::operator()(const Flux::string &s1, const Flux::string &s2) const
{
	return s1.std_str().compare(s2.std_str()) < 0;
}

/** Hash an Flux::string for unordered_map
 * @param s The string
 * @return A hash value for the string
 */
bool Flux::hash::operator()(const Flux::string &s) const
{
	register size_t t = 0;

	for (Flux::string::const_iterator it = s.begin(), it_end = s.end(); it != it_end; ++it)
		t = 5 * t + *it;

	return t;
}

bool ci::ci_char_traits::eq(char c1st, char c2nd)
{
	return ascii_case_insensitive_map[static_cast<unsigned char>(c1st)] == ascii_case_insensitive_map[static_cast<unsigned char>(c2nd)];
}

bool ci::ci_char_traits::ne(char c1st, char c2nd)
{
	return ascii_case_insensitive_map[static_cast<unsigned char>(c1st)] != ascii_case_insensitive_map[static_cast<unsigned char>(c2nd)];
}

bool ci::ci_char_traits::lt(char c1st, char c2nd)
{
	return ascii_case_insensitive_map[static_cast<unsigned char>(c1st)] < ascii_case_insensitive_map[static_cast<unsigned char>(c2nd)];
}

int ci::ci_char_traits::compare(const char *str1, const char *str2, size_t n)
{
	for (unsigned i = 0; i < n; ++i)
	{
		if (ascii_case_insensitive_map[static_cast<unsigned char>(*str1)] > ascii_case_insensitive_map[static_cast<unsigned char>(*str2)])
			return 1;

		if (ascii_case_insensitive_map[static_cast<unsigned char>(*str1)] < ascii_case_insensitive_map[static_cast<unsigned char>(*str2)])
			return -1;

		if (!*str1 || !*str2)
			return 0;

		++str1;
		++str2;
	}
	return 0;
}

const char *ci::ci_char_traits::find(const char *s1, int n, char c)
{
	while (n-- > 0 && ascii_case_insensitive_map[static_cast<unsigned char>(*s1)] != ascii_case_insensitive_map[static_cast<unsigned char>(c)])
		++s1;
	return n >= 0 ? s1 : NULL;
}

/* VS 2008 specific function */
bool ci::hash::operator()(const Flux::string &s1, const Flux::string &s2) const
{
	return s1.ci_str().compare(s2.ci_str()) < 0;
}

/** Hash a ci::string for unordered_map
 * @param s The string
 * @return A hash value for the string
 */
size_t ci::hash::operator()(const ci::string &s) const
{
	register size_t t = 0;

	for (ci::string::const_iterator it = s.begin(), it_end = s.end(); it != it_end; ++it)
		t = 5 * t + ascii_case_insensitive_map[static_cast<const unsigned char>(*it)];

	return t;
}

size_t ci::hash::operator()(const Flux::string &s) const
{
	return operator()(s.ci_str());
}

/** Compare two Flux::strings as ci::strings
 * @param s1 The first string
 * @param s2 The second string
 * @return true if they are equal
 */
bool std::equal_to<ci::string>::operator()(const Flux::string &s1, const Flux::string &s2) const
{
	return s1.ci_str() == s2.ci_str();
}


/** Compare two Flux::strings as ci::strings and find which one is less
 * @param s1 The first string
 * @param s2 The second string
 * @return true if s1 < s2, else false
 */
bool std::less<ci::string>::operator()(const Flux::string &s1, const Flux::string &s2) const
{
	return s1.ci_str().compare(s2.ci_str()) < 0;
}
