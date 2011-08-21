require 'mkmf.rb'

if have_header('dbghelp.h')
  have_type('MINIDUMP_TYPE', 'dbghelp.h')
  have_type('PMINIDUMP_EXCEPTION_INFORMATION', 'dbghelp.h')
  have_type('PMINIDUMP_USER_STREAM_INFORMATION', 'dbghelp.h')
  have_type('PMINIDUMP_CALLBACK_INFORMATION', 'dbghelp.h')
end

have_header('ruby/encoding.h')

# Config::CONFIG["ruby_version"] indicates the ruby API version.
#  1.8   - ruby 1.8.x
#  1.9.1 - ruby 1.9.1 and 1.9.2
#  1.9.x - ruby 1.9.x future version which will break the API compatibility
soname = 'windump_' + Config::CONFIG["ruby_version"].gsub(/\W/, '')

$defs << "-DInit_windump=Init_#{soname}"

create_makefile(soname)
