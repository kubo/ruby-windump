# -*- ruby -*-
require 'fileutils'
require 'rbconfig'
require 'rake/gempackagetask'

def build_and_copy(ruby_cmd = RUBY, clean = false)
  if ruby_cmd == RUBY
    ruby_api_version = Config::CONFIG['ruby_version']
  else
    ruby_api_version = `#{ruby_cmd} -r rbconfig -e "puts Config::CONFIG['ruby_version']"`
  end
  so_basename = "windump_#{ruby_api_version.gsub(/\W/, '')}"
  pwd = Dir.pwd
  if Dir.exists? so_basename
    if clean
      FileUtils.rm_rf so_basename
      Dir.mkdir so_basename
    end
  else
    Dir.mkdir so_basename
  end
  Dir.chdir so_basename
  begin
    puts "chdir #{so_basename}"
    sh ruby_cmd, "../extconf.rb"
    sh /mswin/ =~ RUBY_PLATFORM ? 'nmake' : 'make'
    FileUtils.cp "#{so_basename}.so", '../lib'
  ensure
    Dir.chdir(pwd)
  end
end

task :build do
  build_and_copy
end

task :dist do
  build_and_copy('c:\Ruby\Ruby-187-p302\bin\ruby', true)
  build_and_copy('c:\Ruby\Ruby-191-p430\bin\ruby', true)
  FileUtils.cp 'c:\Program Files\Debugging Tools for Windows (x86)\dbghelp.dll', 'lib'
  Rake::Task[:gem].invoke
end

spec = Gem::Specification.new do |s|
  s.name = 'windump'
  s.version = '0.1'
  s.summary = 'Ruby module to generate a minidump file on segmentation fault'
  s.description = <<EOS
This is useful for ruby-core and extention library developers to fix memory
corruption on Windows. Windows ruby compiled by mingw32, distributed as
RubyInstaller, doesn't generate memory dump on segmentation fault. It is hard
to fix for some kinds of bugs especially when developers cannot reproduce it.

EOS
  s.email = 'kubo@jiubao.org'
  s.homepage = 'https://github.com/kubo/ruby-windump'
  s.authors = ['KUBO Takehiro']
  s.platform = Gem::Platform::CURRENT
  s.files = FileList['README.rdoc', 'lib/*.rb', 'lib/*.so', 'lib/*.dll']
end

Rake::GemPackageTask.new(spec) do |pkg|
end
