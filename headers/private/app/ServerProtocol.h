#ifndef _APPSERVER_PROTOCOL_
#define _APPSERVER_PROTOCOL_

// Server port names. The input port is the port which is used to receive
// input messages from the Input Server. The other is the "main" port for
// the server and is utilized mostly by BApplication objects.
#define SERVER_PORT_NAME "OBappserver"
#define SERVER_INPUT_PORT "OBinputport"

enum
{
// Used for quick replies from the app_server
SERVER_TRUE='_srt',
SERVER_FALSE,
AS_SERVER_BMESSAGE,
AS_SERVER_AREALINK,
AS_SERVER_SESSION,
AS_SERVER_PORTLINK,
AS_CLIENT_DEAD,

// Application definitions
AS_CREATE_APP,
AS_DELETE_APP,
AS_QUIT_APP,

AS_SET_SERVER_PORT,

AS_CREATE_WINDOW,
AS_DELETE_WINDOW,
AS_CREATE_BITMAP,
AS_DELETE_BITMAP,

AS_ACQUIRE_SERVERMEM,
AS_RELEASE_SERVERMEM,
AS_AREA_MESSAGE,

// Cursor definitions
AS_SET_CURSOR_DATA,	
AS_SET_CURSOR_BCURSOR,
AS_SET_CURSOR_BBITMAP,
AS_SET_CURSOR_SYSTEM,

AS_SET_SYSCURSOR_DATA,
AS_SET_SYSCURSOR_BCURSOR,
AS_SET_SYSCURSOR_BBITMAP,
AS_SET_SYSCURSOR_DEFAULTS,
AS_GET_SYSCURSOR,

AS_SHOW_CURSOR,
AS_HIDE_CURSOR,
AS_OBSCURE_CURSOR,
AS_QUERY_CURSOR_HIDDEN,

AS_CREATE_BCURSOR,
AS_DELETE_BCURSOR,

AS_BEGIN_RECT_TRACKING,
AS_END_RECT_TRACKING,

// Window definitions
AS_SHOW_WINDOW,
AS_HIDE_WINDOW,
AS_QUIT_WINDOW,
AS_SEND_BEHIND,
AS_SET_LOOK,
AS_SET_FEEL, 
AS_SET_FLAGS,
AS_DISABLE_UPDATES,
AS_ENABLE_UPDATES,
AS_BEGIN_UPDATE,
AS_END_UPDATE,
AS_NEEDS_UPDATE,
AS_WINDOW_TITLE,
AS_ADD_TO_SUBSET,
AS_REM_FROM_SUBSET,
AS_SET_ALIGNMENT,
AS_GET_ALIGNMENT,
AS_GET_WORKSPACES,
AS_SET_WORKSPACES,
AS_WINDOW_RESIZE,
AS_WINDOW_MOVE,
AS_SET_SIZE_LIMITS,
AS_ACTIVATE_WINDOW,
AS_WINDOW_MINIMIZE,
AS_UPDATE_IF_NEEDED,
_ALL_UPDATED_,	// this should be moved in place of _UPDATE_IF_NEEDED_ in AppDefs.h


// BPicture definitions
AS_CREATE_PICTURE,
AS_DELETE_PICTURE,
AS_CLONE_PICTURE,
AS_DOWNLOAD_PICTURE,

// Font-related server communications
AS_QUERY_FONTS_CHANGED,
AS_UPDATED_CLIENT_FONTLIST,
AS_GET_FAMILY_NAME,
AS_GET_STYLE_NAME,
AS_GET_FAMILY_AND_STYLE,
AS_GET_FONT_DIRECTION,
AS_GET_FONT_BOUNDING_BOX,
AS_GET_TUNED_COUNT,
AS_GET_TUNED_INFO,
AS_GET_FONT_HEIGHT,

AS_QUERY_FONT_FIXED,
AS_SET_FAMILY_NAME,
AS_SET_FAMILY_AND_STYLE,
AS_SET_FAMILY_AND_STYLE_FROM_ID,
AS_SET_FAMILY_AND_FACE,

AS_COUNT_FONT_FAMILIES,
AS_COUNT_FONT_STYLES,

AS_GET_STRING_WIDTH,
AS_GET_STRING_WIDTHS,
AS_GET_EDGES,
AS_GET_ESCAPEMENTS,
AS_GET_BOUNDINGBOXES_CHARS,
AS_GET_BOUNDINGBOXES_STRINGS,
AS_GET_HAS_GLYPHS,
AS_GET_GLYPH_SHAPES,
AS_GET_TRUNCATED_STRINGS,

AS_SET_SYSFONT_PLAIN,
AS_SET_SYSFONT_BOLD,
AS_SET_SYSFONT_FIXED,

// This will be modified. Currently a kludge for the input server until
// BScreens are implemented by the IK Taeam
AS_GET_SCREEN_MODE,

// Global function call defs
AS_SET_UI_COLORS,
AS_GET_UI_COLORS,
AS_GET_UI_COLOR,
AS_SET_DECORATOR,
AS_GET_DECORATOR,
AS_R5_SET_DECORATOR,

AS_COUNT_WORKSPACES,
AS_SET_WORKSPACE_COUNT,
AS_CURRENT_WORKSPACE,
AS_ACTIVATE_WORKSPACE,
AS_SET_SCREEN_MODE,
AS_GET_SCROLLBAR_INFO,
AS_SET_SCROLLBAR_INFO,
AS_IDLE_TIME,
AS_SELECT_PRINTER_PANEL,
AS_ADD_PRINTER_PANEL,
AS_RUN_BE_ABOUT,
AS_SET_FOCUS_FOLLOWS_MOUSE,
AS_FOCUS_FOLLOWS_MOUSE,
AS_SET_MOUSE_MODE,
AS_GET_MOUSE_MODE,

// Hook function messages
AS_WORKSPACE_ACTIVATED,
AS_WORKSPACES_CHANGED,
AS_WINDOW_ACTIVATED,
AS_SCREENMODE_CHANGED,

// Graphics calls
// Are these TRANSACTION codes needed ?
AS_BEGIN_TRANSACTION,
AS_END_TRANSACTION,
AS_SET_HIGH_COLOR,
AS_SET_LOW_COLOR,
AS_SET_VIEW_COLOR,

AS_STROKE_ARC,
AS_STROKE_BEZIER,
AS_STROKE_ELLIPSE,
AS_STROKE_LINE,
AS_STROKE_LINEARRAY,
AS_STROKE_POLYGON,
AS_STROKE_RECT,
AS_STROKE_ROUNDRECT,
AS_STROKE_SHAPE,
AS_STROKE_TRIANGLE,

AS_FILL_ARC,
AS_FILL_BEZIER,
AS_FILL_ELLIPSE,
AS_FILL_POLYGON,
AS_FILL_RECT,
AS_FILL_REGION,
AS_FILL_ROUNDRECT,
AS_FILL_SHAPE,
AS_FILL_TRIANGLE,

AS_MOVEPENBY,
AS_MOVEPENTO,
AS_SETPENSIZE,
AS_DRAW_STRING,
AS_SET_FONT,
AS_SET_FONT_SIZE,

AS_FLUSH,
AS_SYNC,

AS_LAYER_CREATE,
AS_LAYER_DELETE,
AS_LAYER_CREATE_ROOT,
AS_LAYER_DELETE_ROOT,
AS_LAYER_ADD_CHILD, 
AS_LAYER_REMOVE_CHILD,
AS_LAYER_REMOVE_SELF,
AS_LAYER_SHOW,
AS_LAYER_HIDE,
AS_LAYER_MOVE,
AS_LAYER_RESIZE,
AS_LAYER_INVALIDATE,
AS_LAYER_DRAW,

AS_LAYER_GET_TOKEN,
AS_LAYER_ADD,
AS_LAYER_REMOVE,

// View/Layer definitions
AS_LAYER_GET_COORD,
AS_LAYER_SET_FLAGS,
AS_LAYER_SET_ORIGIN,
AS_LAYER_GET_ORIGIN,
AS_LAYER_RESIZE_MODE,
AS_LAYER_CURSOR,
AS_LAYER_BEGIN_RECT_TRACK,
AS_LAYER_END_RECT_TRACK,
AS_LAYER_DRAG_RECT,
AS_LAYER_DRAG_IMAGE,
AS_LAYER_GET_MOUSE_COORDS,
AS_LAYER_SCROLL,
AS_LAYER_SET_LINE_MODE,
AS_LAYER_GET_LINE_MODE,
AS_LAYER_PUSH_STATE,
AS_LAYER_POP_STATE,
AS_LAYER_SET_SCALE,
AS_LAYER_GET_SCALE,
AS_LAYER_SET_DRAW_MODE,
AS_LAYER_GET_DRAW_MODE,
AS_LAYER_SET_BLEND_MODE,
AS_LAYER_GET_BLEND_MODE,
AS_LAYER_SET_PEN_LOC,
AS_LAYER_GET_PEN_LOC,
AS_LAYER_SET_PEN_SIZE,
AS_LAYER_GET_PEN_SIZE,
AS_LAYER_SET_HIGH_COLOR,
AS_LAYER_SET_LOW_COLOR,
AS_LAYER_SET_VIEW_COLOR,
AS_LAYER_GET_COLORS,
AS_LAYER_PRINT_ALIASING,
AS_LAYER_CLIP_TO_PICTURE,
AS_LAYER_CLIP_TO_INVERSE_PICTURE,
AS_LAYER_GET_CLIP_REGION,
AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT,
AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT,
AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT,
AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT,

AS_LAYER_DRAW_STRING,
AS_LAYER_SET_CLIP_REGION,
AS_LAYER_LINE_ARRAY,
AS_LAYER_BEGIN_PICTURE,
AS_LAYER_APPEND_TO_PICTURE,
AS_LAYER_END_PICTURE,
AS_LAYER_COPY_BITS,
AS_LAYER_DRAW_PICTURE,
AS_LAYER_INVAL_RECT,
AS_LAYER_INVAL_REGION,
AS_LAYER_INVERT_RECT,
AS_LAYER_MOVETO,
AS_LAYER_RESIZETO,
AS_LAYER_SET_STATE,
AS_LAYER_SET_FONT_STATE,
AS_LAYER_GET_STATE,
AS_LAYER_SET_VIEW_IMAGE,
AS_LAYER_SET_PATTERN,
AS_SET_CURRENT_LAYER,

// app_server internal communication
AS_ROOTLAYER_SHOW_WINBORDER,
AS_ROOTLAYER_HIDE_WINBORDER
};

#define AS_PATTERN_SIZE 8
#define AS_SET_COLOR_MSG_SIZE 8+4
#define AS_STROKE_ARC_MSG_SIZE 8+6*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_BEZIER_MSG_SIZE 8+8*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_ELLIPSE_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_LINE_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_RECT_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_ROUNDRECT_MSG_SIZE 8+6*sizeof(float)+AS_PATTERN_SIZE
#define AS_STROKE_TRIANGLE_MSG_SIZE 8+10*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_ARC_MSG_SIZE 8+6*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_BEZIER_MSG_SIZE 8+8*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_ELLIPSE_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_RECT_MSG_SIZE 8+4*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_ROUNDRECT_MSG_SIZE 8+6*sizeof(float)+AS_PATTERN_SIZE
#define AS_FILL_TRIANGLE_MSG_SIZE 8+10*sizeof(float)+AS_PATTERN_SIZE

#endif
