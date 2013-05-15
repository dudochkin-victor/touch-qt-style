#include "mlibrarychecker.h"

#include <stdio.h>
#include <link.h>

#include <QDebug>

static bool gChecked = false;
static bool gLinksAgainstMeegoTouchCore = false;

static int callback(struct dl_phdr_info *info, size_t size, void *data)
{
    const char *found = strstr( info->dlpi_name, "libmeegotouchcore" );

    if ( found )
    {
        gLinksAgainstMeegoTouchCore = true;

	// stop walking over shared objects here
	return 1;
    }

    return 0;
}

bool MLibraryChecker::linksAgainstMeegoTouch()
{
    if ( ! gChecked )
    {
        // iterate over the libraries we link against,
        // check them
        dl_iterate_phdr(callback, NULL);

	gChecked = true;
    }

    return gLinksAgainstMeegoTouchCore;
}
