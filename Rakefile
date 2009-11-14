require 'rake/clean'
require 'rake/loaders/makefile'

def execute(command, *args)
	#puts [command, *args].join(' ')
	IO.popen([command, *args]) do |f|
		f.readlines.join('')
	end
end

app = 'mirb'
depends = '.depend.mf'
cc = 'gcc'
cflags = %w[-Wall -std=gnu99 -fstrict-aliasing]
lflags = []
lflags_release = %w[-s]
libs = []
cflags_debug = %w[-g -DDEBUG]
cflags_release = %w[-fexpensive-optimizations -O3 -funroll-loops -fomit-frame-pointer]
sources = [
	'main.c',
	'compiler/**/*.c',
	'runtime/**/*.c'
]

if Rake::Win32.windows?
	cflags << '-DWINDOWS'
	cflags << '-I' << 'vendor/BeaEngine'
	lflags << '-L' << 'vendor/BeaEngine'
	libs << 'BeaEngine'
end

begin
	execute(cc, '--version')
rescue
	raise "Unable to find the C compiler: #{cc}"
end

sources = sources.map do |source| FileList[source] end.flatten
objects = sources.map do |file| file.ext('.o') end

CLEAN.include(app + if Rake::Win32.windows?; '.exe' end)
CLEAN.include(depends)

objects.each do |object|
	CLEAN.include(object)
end

file depends do
	puts "Calculating dependencies..."
	File.open(depends, 'w') do |file|
		sources.each do |source|
  			file << execute(cc, *cflags, '-MM', '-MT', source.ext('.o'), source)
		end
	end
end

import depends

build_type = nil

rule '.o' => ['.c'] do |t|
	puts "Building #{t.source} for #{build_type}..."
	execute cc, *cflags, '-c', t.source, '-o', t.name
	
	unless File.exist? t.name
		raise "Failed to build file #{t.source}."
	end
end

file app => objects do |t|
	puts "Linking #{app}..."
  	execute cc, *lflags, *objects, *(libs.map { |lib| ['-l', lib] }.flatten), '-o', t.name
end

desc "Build an optimized version of #{app}"
task :release do
	build_type = 'release'
	lflags += lflags_release
	cflags += cflags_release
	Rake::Task[app].invoke
end

desc "Build a debug version of #{app}"
task :debug do
	build_type = 'debug'
	cflags += cflags_debug
	Rake::Task[app].invoke
end

task :default => :release
