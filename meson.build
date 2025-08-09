project('websocket','c', default_options: ['c_std=c99','warning_level=everything' ])
add_project_arguments('-Wno-c++-compat', language: 'c')

if host_machine.system() == 'linux'
  lib = library('ws','./websocket.c', './libs/base64.c')
else
  error('only linux supported for now')
endif
executable('base64test','./tests/base64_test.c', './libs/base64.c')
executable('sha1test','./tests/test_sha1_hash.c')
executable('websocket-impl','main.c', link_with:lib)
