project('websocket','c', default_options: ['c_std=c99','warning_level=everything' ])
add_project_arguments('-Wno-c++-compat', language: 'c')
lib = library('ws','./websocket.c', './libs/base64.c' , c_args : ['-mavx','-mavx2', '-msse2'])
executable('websocket-impl','main.c', link_with:lib ,c_args : [ '-mavx','-mavx2', '-msse2'])
