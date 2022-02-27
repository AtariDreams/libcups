/*
 * Global variable access routines for CUPS.
 *
 * Copyright © 2021-2022 by OpenPrinting.
 * Copyright © 2007-2019 by Apple Inc.
 * Copyright © 1997-2007 by Easy Software Products, all rights reserved.
 *
 * Licensed under Apache License v2.0.  See the file "LICENSE" for more
 * information.
 */

/*
 * Include necessary headers...
 */

#include "cups-private.h"
#ifndef _WIN32
#  include <pwd.h>
#endif /* !_WIN32 */


/*
 * Local globals...
 */

#ifdef DEBUG
static int		cups_global_index = 0;
					/* Next thread number */
#endif /* DEBUG */
static cups_threadkey_t cups_globals_key = CUPS_THREADKEY_INITIALIZER;
					/* Thread local storage key */
#ifndef _WIN32
static pthread_once_t	cups_globals_key_once = PTHREAD_ONCE_INIT;
					/* One-time initialization object */
#endif /* !_WIN32 */
static cups_mutex_t	cups_global_mutex = CUPS_MUTEX_INITIALIZER;
					/* Global critical section */


/*
 * Local functions...
 */

#ifdef _WIN32
static void		cups_fix_path(char *path);
#endif /* _WIN32 */
static _cups_globals_t	*cups_globals_alloc(void);
static void		cups_globals_free(_cups_globals_t *g);
#ifndef _WIN32
static void		cups_globals_init(void);
#endif /* !_WIN32 */


/*
 * '_cupsGlobalLock()' - Lock the global mutex.
 */

void
_cupsGlobalLock(void)
{
  cupsMutexLock(&cups_global_mutex);
}


/*
 * '_cupsGlobals()' - Return a pointer to thread local storage
 */

_cups_globals_t *			/* O - Pointer to global data */
_cupsGlobals(void)
{
  _cups_globals_t *cg;			/* Pointer to global data */


#ifndef _WIN32
 /*
  * Initialize the global data exactly once...
  */

  pthread_once(&cups_globals_key_once, cups_globals_init);
#endif /* !_WIN32 */

 /*
  * See if we have allocated the data yet...
  */

  if ((cg = (_cups_globals_t *)cupsThreadGetData(cups_globals_key)) == NULL)
  {
   /*
    * No, allocate memory as set the pointer for the key...
    */

    if ((cg = cups_globals_alloc()) != NULL)
      cupsThreadSetData(cups_globals_key, cg);
  }

 /*
  * Return the pointer to the data...
  */

  return (cg);
}


/*
 * '_cupsGlobalUnlock()' - Unlock the global mutex.
 */

void
_cupsGlobalUnlock(void)
{
  cupsMutexUnlock(&cups_global_mutex);
}


#ifdef _WIN32
/*
 * 'DllMain()' - Main entry for library.
 */

BOOL WINAPI				/* O - Success/failure */
DllMain(HINSTANCE hinst,		/* I - DLL module handle */
        DWORD     reason,		/* I - Reason */
        LPVOID    reserved)		/* I - Unused */
{
  _cups_globals_t *cg;			/* Global data */


  (void)hinst;
  (void)reserved;

  switch (reason)
  {
    case DLL_PROCESS_ATTACH :		/* Called on library initialization */
        InitializeCriticalSection(&cups_global_mutex);

        if ((cups_globals_key = TlsAlloc()) == TLS_OUT_OF_INDEXES)
          return (FALSE);
        break;

    case DLL_THREAD_DETACH :		/* Called when a thread terminates */
        if ((cg = (_cups_globals_t *)TlsGetValue(cups_globals_key)) != NULL)
          cups_globals_free(cg);
        break;

    case DLL_PROCESS_DETACH :		/* Called when library is unloaded */
        if ((cg = (_cups_globals_t *)TlsGetValue(cups_globals_key)) != NULL)
          cups_globals_free(cg);

        TlsFree(cups_globals_key);
        DeleteCriticalSection(&cups_global_mutex);
        break;

    default:
        break;
  }

  return (TRUE);
}
#endif /* _WIN32 */


/*
 * 'cups_globals_alloc()' - Allocate and initialize global data.
 */

static _cups_globals_t *		/* O - Pointer to global data */
cups_globals_alloc(void)
{
  _cups_globals_t *cg = malloc(sizeof(_cups_globals_t));
					/* Pointer to global data */
#ifdef _WIN32
  HKEY		key;			/* Registry key */
  DWORD		size;			/* Size of string */
  static char	installdir[1024] = "",	/* Install directory */
		confdir[1024] = "",	/* Server root directory */
		localedir[1024] = "";	/* Locale directory */
#endif /* _WIN32 */


  if (!cg)
    return (NULL);

 /*
  * Clear the global storage and set the default encryption and password
  * callback values...
  */

  memset(cg, 0, sizeof(_cups_globals_t));
  cg->encryption     = (http_encryption_t)-1;
  cg->password_cb    = (cups_password_cb_t)_cupsGetPassword;
  cg->trust_first    = -1;
  cg->any_root       = -1;
  cg->expired_certs  = -1;
  cg->validate_certs = -1;

#ifdef DEBUG
 /*
  * Friendly thread ID for debugging...
  */

  cg->thread_id = ++ cups_global_index;
#endif /* DEBUG */

 /*
  * Then set directories as appropriate...
  */

#ifdef _WIN32
  if (!installdir[0])
  {
   /*
    * Open the registry...
    */

    strlcpy(installdir, "C:/Program Files/cups.org", sizeof(installdir));

    if (!RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\cups.org", 0, KEY_READ, &key))
    {
     /*
      * Grab the installation directory...
      */

      char  *ptr;			/* Pointer into installdir */

      size = sizeof(installdir);
      RegQueryValueExA(key, "installdir", NULL, NULL, installdir, &size);
      RegCloseKey(key);

      for (ptr = installdir; *ptr;)
      {
        if (*ptr == '\\')
        {
          if (ptr[1])
            *ptr++ = '/';
          else
            *ptr = '\0';		/* Strip trailing \ */
        }
        else if (*ptr == '/' && !ptr[1])
          *ptr = '\0';			/* Strip trailing / */
        else
          ptr ++;
      }
    }

    snprintf(confdir, sizeof(confdir), "%s/conf", installdir);
    snprintf(localedir, sizeof(localedir), "%s/locale", installdir);
  }

  if ((cg->cups_datadir = getenv("CUPS_DATADIR")) == NULL)
    cg->cups_datadir = installdir;

  if ((cg->cups_serverbin = getenv("CUPS_SERVERBIN")) == NULL)
    cg->cups_serverbin = installdir;

  if ((cg->cups_serverroot = getenv("CUPS_SERVERROOT")) == NULL)
    cg->cups_serverroot = confdir;

  if ((cg->cups_statedir = getenv("CUPS_STATEDIR")) == NULL)
    cg->cups_statedir = confdir;

  if ((cg->localedir = getenv("LOCALEDIR")) == NULL)
    cg->localedir = localedir;

  cg->home = getenv("USERPROFILE");

#else
#  ifdef HAVE_GETEUID
  if ((geteuid() != getuid() && getuid()) || getegid() != getgid())
#  else
  if (!getuid())
#  endif /* HAVE_GETEUID */
  {
   /*
    * When running setuid/setgid, don't allow environment variables to override
    * the directories...
    */

    cg->cups_datadir    = CUPS_DATADIR;
    cg->cups_serverbin  = CUPS_SERVERBIN;
    cg->cups_serverroot = CUPS_SERVERROOT;
    cg->cups_statedir   = CUPS_STATEDIR;
    cg->localedir       = CUPS_LOCALEDIR;
  }
  else
  {
   /*
    * Allow directories to be overridden by environment variables.
    */

    if ((cg->cups_datadir = getenv("CUPS_DATADIR")) == NULL)
      cg->cups_datadir = CUPS_DATADIR;

    if ((cg->cups_serverbin = getenv("CUPS_SERVERBIN")) == NULL)
      cg->cups_serverbin = CUPS_SERVERBIN;

    if ((cg->cups_serverroot = getenv("CUPS_SERVERROOT")) == NULL)
      cg->cups_serverroot = CUPS_SERVERROOT;

    if ((cg->cups_statedir = getenv("CUPS_STATEDIR")) == NULL)
      cg->cups_statedir = CUPS_STATEDIR;

    if ((cg->localedir = getenv("LOCALEDIR")) == NULL)
      cg->localedir = CUPS_LOCALEDIR;

    cg->home = getenv("HOME");

#  ifdef __APPLE__ /* Sandboxing now exposes the container as the home directory */
    if (cg->home && strstr(cg->home, "/Library/Containers/"))
      cg->home = NULL;
#  endif /* !__APPLE__ */
  }

  if (!cg->home)
  {
    struct passwd	pw;		/* User info */
    struct passwd	*result;	/* Auxiliary pointer */

    getpwuid_r(getuid(), &pw, cg->pw_buf, PW_BUF_SIZE, &result);
    if (result)
      cg->home = _cupsStrAlloc(pw.pw_dir);
  }
#endif /* _WIN32 */

  return (cg);
}


/*
 * 'cups_globals_free()' - Free global data.
 */

static void
cups_globals_free(_cups_globals_t *cg)	/* I - Pointer to global data */
{
  _cups_buffer_t	*buffer,	/* Current read/write buffer */
			*next;		/* Next buffer */


  if (cg->last_status_message)
    _cupsStrFree(cg->last_status_message);

  for (buffer = cg->cups_buffers; buffer; buffer = next)
  {
    next = buffer->next;
    free(buffer);
  }

  cupsArrayDelete(cg->leg_size_lut);
  cupsArrayDelete(cg->ppd_size_lut);
  cupsArrayDelete(cg->pwg_size_lut);

  httpClose(cg->http);

#ifdef HAVE_TLS
  _httpFreeCredentials(cg->tls_credentials);
#endif /* HAVE_TLS */

  cupsFileClose(cg->stdio_files[0]);
  cupsFileClose(cg->stdio_files[1]);
  cupsFileClose(cg->stdio_files[2]);

  free(cg->raster_error.start);
  free(cg);
}


#ifndef _WIN32
/*
 * 'cups_globals_init()' - Initialize environment variables.
 */

static void
cups_globals_init(void)
{
 /*
  * Register the global data for this thread...
  */

  pthread_key_create(&cups_globals_key, (void (*)(void *))cups_globals_free);
}
#endif /* !_WIN32 */
