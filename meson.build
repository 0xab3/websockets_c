project('websocket','c', default_options: ['c_std=c99','warning_level=everything' ])

lib=library('ws','./websocket.c' , c_args : ['-DBS_IMPLEMENTATION'])
executable('websocket-impl','main.c', link_with:lib )
