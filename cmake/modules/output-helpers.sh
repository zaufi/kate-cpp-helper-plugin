#!/bin/bash
#
# Pretty spam helper functions.
# Reuse Gentoo output subsystem if available.
#

# Get Gentoo style spam functions
test -f /etc/init.d/functions.sh && source /etc/init.d/functions.sh

# Declare homecooked (simplified) spam functions if not available on this system
if test "${RC_GOT_FUNCTIONS}" != "yes"; then
ebegin()
{
    echo "${indent_level}$1"
}
eend()
{
    echo
}
eindent()
{
    indent_level="    "
}
eoutdent()
{
    indent_level=""
}
einfo()
{
    echo "* $1"
}
eerror()
{
    echo "! $1"
}
fi
