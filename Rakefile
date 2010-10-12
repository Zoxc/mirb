require_relative '../reno-0.2/reno'

include Reno

output = 'mirb'

package = Package.new do
	# name and version
	name 'mirb'
	version '0.1.0'
	
	# setup toolchains
	
	set Toolchain::Optimization, :balanced
	set Toolchain::Exceptions, :none
	
	set Toolchain::Libraries, 'vendor/gc/.libs/gc'
	set Toolchain::Libraries, 'vendor/BeaEngine/BeaEngine'
	set Toolchain::Libraries, 'msvcrt'
	
	set Languages::C::Includes, 'vendor'
	
	use Toolchain::GNU
	
	# languages
	use Assembly::WithCPP
	c = use Languages::C
	c.std 'c99'
	c.define 'WIN_SEH'
	c.define 'DEBUG'
	cxx = use Languages::CXX
	cxx.std 'c++0x'
	
	# files
	files = collect('main.c', 'compiler/**/*.c', 'runtime/**/*.c', 'runtime/**/*.S')
	
	# convert all files to assembly for debugging purposes
	files = files.convert(Assembly)
	
	files.merge(Executable).name(output)
end

task :build do
	package.run
end

task :default => :build
