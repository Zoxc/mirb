module Builder	
	class Nasm < Compiler
		def do_compile(source)
			Builder.execute @command, *args(source.settings), File.join(source.package.base, source.name), '-o', source.output
		end
	end
	
	NASM = Nasm.new('nasm', 'nasm', [:NASM_FLAGS], '-h')
end
