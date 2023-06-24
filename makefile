# ----------------------------
# Makefile Options
# ----------------------------

NAME = RAYCAST3
ICON = icon.png
DESCRIPTION = "Raycasting demo"
COMPRESSED = NO
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
