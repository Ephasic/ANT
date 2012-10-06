/* Arbitrary Navn Tool -- Signal Processing
 *
 * (C) 2011-2012 Azuru
 * Contact us at Development@Azuru.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of CIA.vc by Micah Dowty
 * Based on the original code of Anope by The Anope Team.
 */

#include "includes.h"
#include "module.h"
#include "bot.h"
#include "textfile.h"
// #include <cassert>

/** Segmentation Fault Handler
 * \fn void HandleSegfault(module *m)
 * \brief A segfault handler to report what happened and where it happened.
 * \param module the module class in which the segfault happened include
 */
void HandleSegfault(Module *m)
{
#ifdef HAVE_BACKTRACE
    void *array[10];
    char **strings;
    char tbuf[256];
    Flux::string mbuf;
    size_t size;
    time_t now = time(NULL);

    size = backtrace(array, 10);
    if(TextFile::IsFile("SEGFAULT.log"))
    Delete("SEGFAULT.log");
    std::stringstream slog;
    std::fstream sslog("SEGFAULT.log", std::ifstream::out | std::ifstream::app);
    if(sslog.is_open())
    {
	struct utsname uts;
	if(uname(&uts) < 0)
	    throw CoreException("uname() Error");

	strftime(tbuf, sizeof(tbuf), "[%b %d %H:%M:%S %Y]", localtime(&now));
	slog << "====================== Segmentation Fault ======================" << std::endl;
	slog << "Please report this bug to http://bugs.Azuru.net/ and submit a bug report." << std::endl;
	slog << "Please note that the Azuru developers may ask you to re-run this under gdb!" << std::endl;
	slog << "Time of crash: " << tbuf << std::endl;
	slog << Flux::string(COMPILED_NAME).toupper() << " version: " << VERSION_FULL << std::endl;
	slog << "System info: " << uts.sysname << " " << uts.nodename << " " <<  uts.release << " " << uts.machine << std::endl;
	slog << "System version: " << uts.version << std::endl;
	slog << "C++ Version: " << __VERSION__ << std::endl;
	slog << "Socket Buffer: " << LastBuf << std::endl;
	slog << "Location: " << segv_location << std::endl;

	if(m)
	{
	    slog << "Module: " << m->name << std::endl;
	    slog << "Module Version: " << m->GetVersion() << std::endl;
	    slog << "Module Author: " << m->GetAuthor() << std::endl;
	}

	for(auto it : Modules)
	    mbuf += it->name+" ";

	mbuf.trim();
	slog << "Modules Loaded: " << (mbuf.empty()?"None":mbuf) << std::endl;
	strings = backtrace_symbols(array, size);

	for(unsigned i=1; i < size; i++)
	    slog << "BackTrace(" << (i - 1) << "): " << strings[i] << std::endl;

	free(strings);
	slog << "======================== END OF REPORT ==========================" << std::endl;
	sslog << slog.str() << std::endl; //Write to SEGFAULT.log
	sslog.close(); //Close pointer to SEGFAULT.log
	std::cout << slog.str(); //Write to terminal.
	std::cout.flush(); //Clear output
	if(m)
	    Log() << "Segmentation Fault in module " << m->name << ", please review SEGFAULT.log";
	else
	    Log(LOG_SILENT) << "\033[0mSegmentation Fault, Please read SEGFAULT.log";
    }
    else
	throw CoreException("Segmentation Fault, cannot write backtrace!");
#else
    Log(LOG_SILENT) << "Segmentation Fault";
    printf("\033[0mOh no! A Segmentation Fault has occured!\n");
    printf("This system does not support backtracing, please use gdb or a similar debugger!\n");
    printf("Please follow these instructions on how to file a bug report of Flux-Net:\n");
    printf("1) type \"gdb ant\"\n2) type \"r -n --protocoldebug\"\n3) Cause the program to crash\n4) Type \"bt full\" and copy and paste the output to http://www.pastebin.com/\n5) File a bug report at http://flux-net.net/bugs/\n");
#endif
}
/** Terminal Signal Handler
 * \fn void sigact(int sig)
 * \brief A handler which handles what to do when we receive a signal.
 * \param int the signal we received.
 */
void sigact(int sig)
{
    FOREACH_MOD(I_OnSignal, OnSignal(sig));
    switch(sig)
    {
	case SIGPIPE:
	    signal(sig, SIG_IGN);
	    Log(LOG_RAWIO) << "Received SIGPIPE, must be a dead socket somewhere..";
	    break; //Ignore SIGPIPE
	case SIGHUP:
	    signal(sig, SIG_IGN);
	    Rehash();
	    break;
	case SIGSEGV:
	    Log(LOG_RAWIO) << "Received SIGSEGV, Segmentation Fault caught.\033[0m";
	    /* this is where the module stack needs to be */
	    #ifdef HAVE_SETJMP_H
	    if(LastRunModule)
	    {
		HandleSegfault(LastRunModule);
		ModuleHandler::Unload(LastRunModule);
		Log() << "Attempting to restore Stack to before Crash";
		longjmp(sigbuf, -1);
		Log() << "Finished restoring Stack";
		break;
	    }
	    #endif
	    HandleSegfault(NULL);
	    signal(sig, SIG_DFL);
	    for(auto it : Networks) // wouldn't hurt to try and exit saying we segfaulted.
		it.second->Disconnect("Segmentation Fault");
	    exit(SIGSEGV);
	    break;
	case SIGINT:
	case SIGKILL:
	case SIGTERM:
	    signal(sig, SIG_IGN);
	    signal(SIGHUP, SIG_IGN);
	    quitmsg = "Received Signal SIGTERM, exiting..";
	    Log(LOG_WARN) << quitmsg;
	    for(auto it : Networks)
		if(it.second->b)
		    it.second->Disconnect(quitmsg);
	    quitting = true;
	    break;
	default:
	    Log() << "Received weird signal from terminal. Sig Number: " << sig;
	    signal(sig, SIG_IGN);
    }
}
/** Signal Initializer
 * \fn void InitSignals()
 * \brief Initialize the signals we catch in this void
 */
void InitSignals()
{
    signal(SIGTERM, sigact);
    signal(SIGINT, sigact);
    signal(SIGHUP, sigact);
    signal(SIGSEGV, sigact);
    signal(SIGPIPE, sigact);
}