#include "opcodes.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		void MoveOp::def_use(BasicBlock &block)
		{
			block.def(dst);
			block.use(src);
		}
		
		void LoadOp::def_use(BasicBlock &block)
		{
			block.def(var);
		}
		
		void LoadRawOp::def_use(BasicBlock &block)
		{
			block.def(var);
		}
		
		void PushOp::def_use(BasicBlock &block)
		{
			block.use(var);
		}
		
		void ClosureOp::def_use(BasicBlock &block)
		{
			block.def(var);
			block.use(self);
		}
		
		void ClassOp::def_use(BasicBlock &block)
		{
			block.def(var);
			block.use(self);
			block.use(super);
		}
		
		void ModuleOp::def_use(BasicBlock &block)
		{
			block.def(var);
			block.use(self);
		}
		
		void MethodOp::def_use(BasicBlock &block)
		{
			block.use(self);
		}
		
		void CallOp::def_use(BasicBlock &block)
		{
			block.def(var);
			block.use(obj);

			if(this->block)
				block.use(this->block);
		}
		
		void SuperOp::def_use(BasicBlock &block)
		{
			block.def(var);
			block.use(self);
			block.use(module);
			block.use(method);

			if(this->block)
				block.use(this->block);
		}
		
		void GetIVarOp::def_use(BasicBlock &block)
		{
			block.def(var);
			block.use(self);
		}
		
		void SetIVarOp::def_use(BasicBlock &block)
		{
			block.use(var);
			block.use(self);
		}
		
		void GetConstOp::def_use(BasicBlock &block)
		{
			block.def(var);
			block.use(obj);
		}
		
		void SetConstOp::def_use(BasicBlock &block)
		{
			block.use(var);
			block.use(obj);
		}
		
		void BranchIfOp::def_use(BasicBlock &block)
		{
			block.use(var);
		}
		
		void BranchUnlessOp::def_use(BasicBlock &block)
		{
			block.use(var);
		}
		
		void ReturnOp::def_use(BasicBlock &block)
		{
			block.use(var);
		}
		
		void UnwindReturnOp::def_use(BasicBlock &block)
		{
			block.use(var);
		}
		
		void UnwindBreakOp::def_use(BasicBlock &block)
		{
			block.use(var);
		}
		
		void ArrayOp::def_use(BasicBlock &block)
		{
			block.def(var);
		}
		
		void StringOp::def_use(BasicBlock &block)
		{
			block.def(var);
		}
		
		void InterpolateOp::def_use(BasicBlock &block)
		{
			block.def(var);
		}
	};
};
