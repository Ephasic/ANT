/* Arbitrary Navn Tool -- Miscellaneous functions and routines.
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */
#include "module.h"
#include "bot.h"
#include "INIReader.h"

//General misc functions
/**Random Number Generator
 * This will generate a random number x is start number, y is the stop number.
 * @param randint(int x, int y)
 */
int randint(int x, int y)
{
    srand(time(NULL));
    return rand()%(y-x+1)+x;
}

/**
 * \fn IsoHost(const Flux::string &fullhost)
 * \param fullhost A Flux::string containing the full host of an irc message
 */
IsoHost::IsoHost(const Flux::string &fullhost)
{
    nick = fullhost.isolate(':','!');
    raw = fullhost;
    host = fullhost.isolate('@',' ');
    ident = fullhost.isolate('!','@');
}

void Fork()
{
    if (!nofork && InTerm())
    {
	int i = fork();

	if(i > 0)
	{
	    Log(LOG_TERMINAL) << "ANT Commit System v" << VERSION << " Started";
	    Log(LOG_TERMINAL) << "Forking to background. PID: " << i << "\033[0m";
	    FOREACH_MOD(I_OnFork, OnFork(i));
	    exit(0);
	}

	if(isatty(fileno(stdout)))
	    fclose(stdout);
	if(isatty(fileno(stdin)))
	    fclose(stdin);
	if(isatty(fileno(stderr)))
	    fclose(stderr);
	if(setpgid(0, 0) < 0)
	    throw CoreException("Unable to setpgid()");
	else if(i == -1)
	    Log() << "Error, unable to fork: " << strerror(errno);
    }
    else
	Log() << "ANT Commit System Started, PID: " << getpid() << Config->LogColor;
}

Flux::string CondenseVector(const Flux::vector &params)
{
    Flux::string ret;
    for(auto it : params)
	ret += it + " ";
    ret.trim();
    return ret;
}

Flux::string Flux::RandomNickString(size_t length)
{
    Flux::string randomchars;
    srand(time(NULL));
    for(unsigned i=0; i < length; ++i)
    {
	top:
	char c = (char) (rand() % ('z' - '0' + 1) + '0');
	if(isalphibeticnum(c))
	    randomchars += c;
	else
	    goto top;
    }
    return randomchars;
}

Flux::string Flux::RandomString(size_t length)
{
    Flux::string randomchars;
    srand((unsigned)time(NULL));
    for(unsigned i=0; i < length; ++i)
	randomchars += (char) (rand() % ('z' - '0' + 1) + '0');
    return randomchars;
}

/**
 * \fn Flux::string strip_mirc_codes(const Flux::string &str)
 * \brief Attempts to remove all mIRC color codes from an IRC message
 * \param buffer A Flux::string containing mIRC color codes
 * This function was an import of InspIRCd's mirc color stripper and was
 * rewritten into C++ for compatibility with Flux::strings
 */

// The following function was taken from m_stripcolor.cpp from InspIRCd 2.0 and
// modified to fit this project.
Flux::string StripColors(const Flux::string &input)
{
    /* refactor this completely due to SQUIT bug since the old code would strip last char and replace with \0 --peavey */
    Flux::string sentence = input;
    int seq = 0;
    Flux::string::iterator i,safei;
    for (i = sentence.begin(); i != sentence.end();)
    {
	if ((*i == 3))
	    seq = 1;
	else if (seq && (( ((*i >= '0') && (*i <= '9')) || (*i == ',') ) ))
	{
	    seq++;
	    if ( (seq <= 4) && (*i == ',') )
		seq = 1;
	    else if (seq > 3)
		seq = 0;
	}
	else
	    seq = 0;

	if (seq || ((*i == 2) || (*i == 15) || (*i == 22) || (*i == 21) || (*i == 31)))
	{
	    if (i != sentence.begin())
	    {
		safei = i;
		--i;
		sentence.erase(safei);
		++i;
	    }
	    else
	    {
		sentence.erase(i);
		i = sentence.begin();
	    }
	}
	else
	    ++i;
    }

    return sentence;
}

Flux::string Flux::Sanitize(const Flux::string &string)
{
    static struct special_chars
    {
	Flux::string character;
	Flux::string replace;
	special_chars(const Flux::string &c, const Flux::string &r) : character(c), replace(r) { }
    }
    special[] = {
	special_chars("  ", " "),
	special_chars("\n",""),
	special_chars("\002",""),
	special_chars("\035",""),
	special_chars("\037",""),
	special_chars("\026",""),
	special_chars("\001",""),
	special_chars("\r", ""),
	special_chars("","")
    };

    Flux::string ret = string.c_str();
    ret = StripColors(ret);
    for(int i = 0; special[i].character.empty() == false; ++i)
	ret = ret.replace_all_cs(special[i].character, special[i].replace);
    return ret.c_str();
}

Flux::string printfify(const char *fmt, ...)
{
    if(fmt)
    {
	va_list args;
	char buf[BUFSIZE];
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	return buf;
    }
    else
	return fmt;
}

/**
 * \fn std::vector<Flux::string> StringVector(const Flux::string &src, char delim)
 * \brief creates a vector that breaks down a string word-by-word using the delim as the seperater
 * \param src The source string for it to break down
 * \param delim The char used to seperate the words in the source string
 */
std::vector<Flux::string> ParamitizeString(const Flux::string &src, char delim)
{
    sepstream tok(src, delim);
    Flux::string token;
    std::vector<Flux::string> ret;

    while(tok.GetToken(token))
	ret.push_back(token);

    return ret;
}
/** Check if a file exists
 * \fn bool InTerm()
 * \brief returns if the
 * \return true if the file exists, false if it doens't
 */
bool InTerm()
{
    return isatty(fileno(stdout) && isatty(fileno(stdin)) && isatty(fileno(stderr)));
}

/**
 * \fn Flux::string duration(const time_t &t)
 * Expresses in a string the period of time represented by a given amount
 * of seconds (with days/hours/minutes).
 * \param seconds time in seconds
 * \return buffer
 */
Flux::string duration(const time_t &t)
{
    /* We first calculate everything */
    time_t days = (t / 86400);
    time_t hours = (t / 3600) % 24;
    time_t minutes = (t / 60) % 60;
    time_t seconds = (t) % 60;

    if (!days && !hours && !minutes)
	return value_cast<Flux::string>(seconds) + " " + (seconds != 1 ? "seconds" : "second");
    else
    {
	bool need_comma = false;
	Flux::string buffer;
	if (days)
	{
	    buffer = value_cast<Flux::string>(days) + " " + (days != 1 ? "days" : "day");
	    need_comma = true;
	}

	if (hours)
	{
	    buffer += need_comma ? ", " : "";
	    buffer += value_cast<Flux::string>(hours) + " " + (hours != 1 ? "hours" : "hour");
	    need_comma = true;
	}

	if (minutes)
	{
	    buffer += need_comma ? ", " : "";
	    buffer += value_cast<Flux::string>(minutes) + " " + (minutes != 1 ? "minutes" : "minute");
	}
	return buffer;
    }
}

Flux::string do_strftime(const time_t &t, bool short_output)
{
    tm tm = *localtime(&t);
    char buf[BUFSIZE];
    strftime(buf, sizeof(buf), "%b %d %H:%M:%S %Y %Z", &tm);
    if (short_output)
	return buf;
    if (t < time(NULL))
	return Flux::string(buf) + " " + printfify("(%s ago)", duration(time(NULL) - t).c_str());
    else
	return Flux::string(buf) + " " + printfify("(%s from now)", duration(t - time(NULL)).c_str());
}


/* butt-plug?
 * http://www.albinoblacksheep.com/flash/plugs */