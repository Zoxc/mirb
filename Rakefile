require 'rake/clean'
require 'rake/loaders/makefile'

def execute(command, *args)
	IO.popen([command, *args]) do |f|
		f.readlines.join('')
	end
end

cc = 'gcc'
cflags = %w[-Wall -std=gnu99 -fstrict-aliasing]
cflags_debug = %w[-g -DDEBUG]
cflags_release = %w[-fexpensive-optimizations -O3 -funroll-loops -fomit-frame-pointer]

begin
	execute(cc, '--version')
rescue
	raise "Unable to find the C compiler: #{cc}"
end

sources = [
	FileList['main.c'],
	FileList['compiler/**/*.c'],
	FileList['runtime/**/*.c']
].flatten!

objects = sources.map do |file| file.ext('.o') end

app = 'mirb'
depends = '.depend.mf'

file depends do
	puts "Calculating dependencies..."
	File.open(depends, 'w') do |file|
		sources.each do |source|
  			file << execute(cc, *cflags, '-MM', '-MT', source.ext('.o'), source)
		end
	end
end

import depends

CLEAN.include("main.o")
CLEAN.include("compiler/**/*.o")
CLEAN.include("runtime/**/*.o")
CLEAN.include(app)
CLEAN.include(depends)

build_type = nil

rule '.o' => ['.c'] do |t|
	puts "Building #{t.source} for #{build_type}..."
	execute cc, *cflags, '-c', t.source, '-o', t.name
end

file app => objects do |t|
	puts "Linking #{app}..."
  	execute cc, *cflags, *objects, '-o', t.name
end

desc "Build an optimized version of #{app}"
task :release do
	build_type = 'release'
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
