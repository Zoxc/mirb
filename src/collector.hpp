#pragma once
#include "common.hpp"
#include <Prelude/FastList.hpp>
#include "value.hpp"

namespace Mirb
{
	class BasicObjectHeader
	{
		private:
			#ifdef DEBUG
				size_t magic;
			#endif

			const Value::Type type;
			static bool inverted;

			friend class Collector;
		public:
			ListEntry<BasicObjectHeader> header_entry;

			BasicObjectHeader(Value::Type type);
			
			Value::Type get_type();
	};

	class ObjectHeader:
		public BasicObjectHeader
	{
		private:
		public:
			ObjectHeader(Value::Type type) : BasicObjectHeader(type) {}
	};

	// PinnedHeader are objects with a fixed memory address

	class PinnedHeader:
		public BasicObjectHeader
	{
		public:
			PinnedHeader(Value::Type type) : BasicObjectHeader(type) {}
	};

	class Collector
	{
		private:
			template<class T> static void *allocate_object()
			{
				allocated += sizeof(T);


				if(allocated > 0x1000)
					collect();

				return std::malloc(sizeof(T));
			}
			
			template<class T> static T *setup_object(T *object)
			{
				(void)static_cast<ObjectHeader *>((T *)0); // Make sure the object contains an header

				object_list.append(object);

				return object;
			}

			template<class T> static T *setup_pinned_object(T *object)
			{
				(void)static_cast<PinnedHeader *>((T *)0); // Make sure the object contains an header

				pinned_object_list.append(object);

				return object;
			}
			
			static size_t allocated;
			static FastList<ObjectHeader, BasicObjectHeader, &BasicObjectHeader::header_entry> object_list;
			static FastList<PinnedHeader, BasicObjectHeader, &BasicObjectHeader::header_entry> pinned_object_list;

		public:
			static void collect();

			template<class T> static T *allocate_pinned()
			{
				return setup_pinned_object<T>(new (allocate_object<T>()) T());
			}
			
			template<class T, typename Arg1> static T *allocate_pinned(Arg1&& arg1)
			{
				return setup_pinned_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1)));
			}
			
			template<class T> static T *allocate()
			{
				return setup_object<T>(new (allocate_object<T>()) T());
			}
			
			template<class T, typename Arg1> static T *allocate(Arg1&& arg1)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1)));
			}
			
			template<class T, typename Arg1, typename Arg2> static T *allocate(Arg1&& arg1, Arg2&& arg2)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2)));
			}
			
			template<class T,  typename Arg1, typename Arg2, typename Arg3> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg4&& arg5)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7, Arg8&& arg8)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7), std::forward<Arg8>(arg8)));
			}
	};
};
