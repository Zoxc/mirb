require 'rake'
require 'rexml/document'

module Builder	
	def self.execute(command, *args)
		#Kernel.puts [command, *args].join(' ')
		result = nil
		IO.popen([command, *args]) do |f|
			result = f.readlines.join('')
		end
		raise "#{command} failed with error code #{$?.exitstatus}" if $?.exitstatus != 0
		result
	end
	
	def self.clean(filename)
		if File.exists?(filename)
			File.delete(filename)
			begin
				dir = File.dirname(filename)
				while File.exists?(dir)
					Dir.rmdir(dir)
					dir = File.dirname(dir)
				end
			rescue SystemCallError
			end
		end
	end
	
	def self.puts(setting, *messages)
		Kernel.puts *messages
	end
	
	class Compiler
		attr :command, true
		def initialize(name, command, args = [], helparg = '--version')
			@name = name
			@command = command
			@args = args
			@located = false
			@helparg = helparg
		end
		
		def output(source)
			source.name + '.o'
		end
		
		def args(settings)
			@args.map do |arg|
				if arg.class == Symbol
					settings[arg]
				else
					arg
				end
			end.flatten!
		end
		
		def require
			return true if @located
			begin
				Builder.execute(@command, @helparg)
				@located = true
			rescue
				raise "Unable to find #{@name}"
			end
		end
		
		def load_dependencies(source, node)
			source.dependencies = node.elements.map { |dependency| dependency.attribute('name').value }
		end
		
		def get_dependencies(source, node)	
			doc = node.add_element 'source', {'name' => source.name, 'time' => File.stat(File.join(source.package.base, source.name)).mtime}
			doc.add_element 'dependency', {'name' => source.name}
			source.dependencies = []
		end
		
		def compile(source)
			if File.exists?(source.output)
				mtime = File.stat(source.output).mtime
				return false if source.dependencies.all? { |dependency| mtime >= File.stat(File.join(source.package.base, dependency)).mtime }
			end
			
			Builder.puts source.settings, "Compiling #{source.name} for #{source.settings.name.downcase} using #{@name}..."
			
			makedirs(File.dirname(source.output))
			
			do_compile(source)
			
			unless File.exist? source.output
				raise "Failed to compile file #{source.name}."
			end
			
			true
		end
	end
	
	class As < Compiler
		def do_compile(source)
			Builder.execute @command, *args(source.settings), '-x', 'assembler', '-c', File.join(source.package.base, source.name), '-o', source.output
		end
	end
	
	class Gcc < Compiler
		def respace(str)
			str.gsub(/#{"__&NBSP;__"}/, ' ')
		end
		
		def process_line(source, line)
			file_tasks, args = line.split(':')
			return if args.nil?
			dependencies = args.split.map { |d| respace(d) }
			file_tasks.strip.split.each do |file_task|
				file_task = respace(file_task)

				source.dependencies += dependencies
			end
		end
		
		def get_dependencies(source, node)	
			Builder.puts source.settings, "Getting dependencies for #{source.name}..."
			
			source.dependencies = []
			
			lines = Builder.execute(@command, *args(source.settings), '-MM', '-MT', source.output, File.join(source.package.base, source.name))
			lines.gsub!(/\\ /, "__&NBSP;__")
			lines.gsub!(/#[^\n]*\n/m, "")
			lines.gsub!(/\\\n/, ' ')
			lines.split("\n").each do |line|
				process_line(source, line)
			end
			
			doc = node.add_element 'source', {'name' => source.name, 'time' => File.stat(File.join(source.package.base, source.name)).mtime}
			
			source.dependencies.each do |dependency|
				doc.add_element 'dependency', {'name' => dependency}
			end
		end
		
		def do_compile(source)
			Builder.execute @command, *args(source.settings), '-c', File.join(source.package.base, source.name), '-o', source.output
		end
	end
	
	AS = As.new('gas', ENV['CC'] || 'gcc', [:FLAGS])
	GCC = Gcc.new('gcc', ENV['CC'] || 'gcc', [:FLAGS, :CFLAGS])
	
	class Settings
		attr_reader :name, :parent

		def initialize(name, parent = nil, settings = nil)
			@name = name
			@parent = parent
			@settings = settings
		end
		
		def []=(name, value)
			@settings[name] = value
		end
		
		def [](name)
			result = @settings[name]
			parent = (@parent[name] if @parent) rescue nil
			if result
				if parent
					case result
						when Array
							return result + parent
						when Hash
							return parent.merge(result)
						else
							return result
					end
				else
					return result
				end
			elsif parent
				return parent
			else
				raise "Unable to find setting #{name}"
			end
		end

	end
	
	module Preset
		BASE = Settings.new('Base', nil, {:OUTPUT => 'build/base', '*.c' => Builder::GCC, '*.asm' => Builder::AS, :FLAGS => [], :CFLAGS => ['-Wall', '-std=gnu99'], :LIBS => [], :LD => ENV['CC'] || 'gcc', :LDFLAGS => []})
		RELEASE = Settings.new('Release', BASE, {:OUTPUT => 'build/release', :CFLAGS => ['-O3', '-funroll-loops', '-fomit-frame-pointer'], :LDFLAGS => ['-s']})
		DEBUG = Settings.new('Debug', BASE, {:OUTPUT => 'build/debug', :FLAGS => ['-g'], :CFLAGS => ['-g', '-DDEBUG']})
	end
	
	class Source
		attr_reader :name, :output, :settings, :package, :compiler
		attr :dependencies, true
		
		def initialize(package, name)
			@name = name
			@package = package
			@settings = @package.settings
			
			begin
				@compiler = @settings['*' + File.extname(@name)]
			rescue
				raise "Unable to find compiler for file #{@name}."
			end
			
			@compiler.require
			@output = File.join(@package.base, @settings[:OUTPUT], @compiler.output(self))
		end
		
		def clean
			Builder.clean(@output)
		end	
	end
	
	class Package
		attr_reader :name, :base, :settings, :output

		def initialize(name, base, pattern_list, output = nil)
			@name = name
			@base = base
			@pattern_list = pattern_list
			@flags = {}
			@target_output = output
		end
		
		def sources
			@pattern_list
		end
		
		def sources=(value)
			@pattern_list = value
		end
		
		def load_dependencies(file)
			raise unless File.exists?(file)

			xml = REXML::Document.new(File.new(file, 'r'))
			package = xml.root
			raise if package.name != 'package'
			raise if package.attribute('name').value != @name
			
			package.elements.each do |source_node|
				filename = source_node.attribute('name').value
				source = @sources.find { |source| File.identical?(source.name, filename) }
				source.compiler.load_dependencies(source, source_node) if source
			end
			
			raise if @sources.any? { |source| !source.dependencies }
		end
		
		def get_dependencies(file)
			xml = REXML::Document.new
			
			package = xml.add_element 'package', {'name' => @name}
			
			@sources.each { |source| source.compiler.get_dependencies(source, package) }
			
			xml.write(File.new(file, 'w'), 4)
		end
		
		def build_sources
			@sources = @pattern_list.map { |source| FileList[source] }.flatten.map { |source| Source.new(self, source) }
		end
		
		def depends_file
			File.join(@base, @settings[:OUTPUT], ".depends.#{@name}.xml")
		end
		
		def compile
			build_sources
			
			makedirs(File.join(@base, @settings[:OUTPUT]))

			begin
				load_dependencies(depends_file)
			rescue
				get_dependencies(depends_file)
			end	
			
			need_linking = false
			
			@sources.each { |source| need_linking = source.compiler.compile(source) || need_linking }
			
			if need_linking || !File.exists?(@output)
				link
				true
			else
				false
			end
		end
		
		def link
			Builder.puts @settings, "Linking #{@name} for #{@settings.name.downcase}..."
			
			Builder.execute @settings[:LD], *@settings[:LDFLAGS], *(@sources.map { |source| source.output }), *(@settings[:LIBS].map { |lib| ['-l', lib] }.flatten), '-o', @output
		end
		
		def clean(settings = Preset::RELEASE)
			@settings = settings
			build_sources
			@sources.each { |source| source.clean }
			Builder.clean(get_output)
			Builder.clean(depends_file)
			true
		end
		
		def get_output
			@output = @target_output or @output = File.join(@base, @settings[:OUTPUT], @name + if Rake::Win32.windows?; '.exe' end.to_s)
		end
		
		def build(settings = Preset::RELEASE)
			@settings = settings
			get_output
			
			compile
		end
	end
end
