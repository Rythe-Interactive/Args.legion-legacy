#pragma once
#include <vector>
#include <functional>

namespace Args::stl
{
	template<typename T>
	class delegate_base;

	template<typename return_type, typename ...parameter_types>
	class delegate_base<return_type(parameter_types...)>
	{
	protected:

		using stub_type = return_type(*)(void* this_ptr, parameter_types...);

		struct invocation_element
		{
			invocation_element() = default;
			invocation_element(void* this_ptr, stub_type aStub) : object(this_ptr), stub(aStub) {}

			void Clone(invocation_element& target) const
			{
				target.stub = stub;
				target.object = object;
			}

			bool operator ==(const invocation_element& other) const
			{
				return other.stub == stub && other.object == object;
			}
			bool operator !=(const invocation_element& other) const
			{
				return other.stub != stub || other.object != object;
			}
			
			void* object = nullptr;
			stub_type stub = nullptr;
		};
	};


	template <typename T> class delegate;
	template <typename T> class multicast_delegate;

	template<typename return_type, typename ...parameter_types>
	class delegate<return_type(parameter_types...)> final : private delegate_base<return_type(parameter_types...)>
	{
	public:
		delegate() = default;

		bool isNull() const
		{
			return invocation.stub == nullptr;
		}

		bool operator ==(void* ptr) const
		{
			return (ptr == nullptr) && this->isNull();
		}
		bool operator !=(void* ptr) const
		{
			return (ptr != nullptr) || (!this->isNull());
		}

		delegate(const delegate& other)
		{
			other.invocation.Clone(invocation);
		}
		template <typename lambda_type>
		delegate(const lambda_type& lambda)
		{
			assign((void*)(&lambda), lambda_stub<lambda_type>);
		}

		delegate& operator =(const delegate& other)
		{
			other.invocation.Clone(invocation);
			return *this;
		}

		template <typename lambda_type>
		delegate& operator =(const lambda_type& instance)
		{
			assign((void*)(&instance), lambda_stub<lambda_type>);
			return *this;
		}

		bool operator == (const delegate& other) const
		{
			return invocation == other.invocation;
		}
		bool operator != (const delegate& other) const
		{
			return invocation != other.invocation;
		}

		bool operator ==(const multicast_delegate<return_type(parameter_types...)>& other) const
		{
			return other == (*this);
		}
		bool operator !=(const multicast_delegate<return_type(parameter_types...)>& other) const
		{
			return other != (*this);
		}

		template <class owner_type, return_type(owner_type::* func_type)(parameter_types...)>
		static delegate create(owner_type* instance)
		{
			return delegate(instance, method_stub<owner_type, func_type>);
		}

		template <class owner_type, return_type(owner_type::* func_type)(parameter_types...) const>
		static delegate create(owner_type const* instance)
		{
			return delegate(const_cast<owner_type*>(instance), const_method_stub<owner_type, func_type>);
		}

		template <return_type(*func_type)(parameter_types...)>
		static delegate create()
		{
			return delegate(nullptr, function_stub<func_type>);
		}

		template <typename lambda_type>
		static delegate create(const lambda_type& instance)
		{
			return delegate((void*)(&instance), lambda_stub<lambda_type>);
		}

		return_type operator()(parameter_types... arguments) const
		{
			return (*invocation.stub)(invocation.object, arguments...);
		}
		return_type invoke(parameter_types... arguments) const
		{
			return (*invocation.stub)(invocation.object, arguments...);
		}

	private:
		delegate(void* anObject, typename delegate_base<return_type(parameter_types...)>::stub_type aStub)
		{
			invocation.object = anObject;
			invocation.stub = aStub;
		}

		void assign(void* anObject, typename delegate_base<return_type(parameter_types...)>::stub_type aStub)
		{
			this->invocation.object = anObject;
			this->invocation.stub = aStub;
		}

		template <class owner_type, return_type(owner_type::* func_type)(parameter_types...)>
		static return_type method_stub(void* this_ptr, parameter_types... arguments)
		{
			owner_type* p = static_cast<owner_type*>(this_ptr);
			return (p->*func_type)(arguments...);
		}

		template <class owner_type, return_type(owner_type::* func_type)(parameter_types...) const>
		static return_type const_method_stub(void* this_ptr, parameter_types... arguments)
		{
			owner_type* const p = static_cast<owner_type*>(this_ptr);
			return (p->*func_type)(arguments...);
		}

		template <return_type(*func_type)(parameter_types...)>
		static return_type function_stub(void* this_ptr, parameter_types... arguments)
		{
			return (func_type)(arguments...);
		}

		template <typename lambda_type>
		static return_type lambda_stub(void* this_ptr, parameter_types... arguments)
		{
			lambda_type* p = static_cast<lambda_type*>(this_ptr);
			return (p->operator())(arguments...);
		}

		friend class multicast_delegate<return_type(parameter_types...)>;
		typename delegate_base<return_type(parameter_types...)>::invocation_element invocation;
	};

	template<typename return_type, typename ...parameter_types>
	class multicast_delegate<return_type(parameter_types...)> final : private delegate_base<return_type(parameter_types...)>
	{
	public:

		multicast_delegate() = default;
		~multicast_delegate()
		{
			for (auto& element : invocationList)
				delete element;

			invocationList.clear();
		}

		bool isNull() const
		{
			return invocationList.size() < 1;
		}

		bool operator ==(void* ptr) const
		{
			return (ptr == nullptr) && this->isNull();
		}
		bool operator !=(void* ptr) const
		{
			return (ptr != nullptr) || (!this->isNull());
		}

		size_t size() const { return invocationList.size(); }

		multicast_delegate& operator =(const multicast_delegate&) = delete;
		multicast_delegate(const multicast_delegate&) = delete;

		bool operator ==(const multicast_delegate& other) const
		{
			if (invocationList.size() != other.invocationList.size())
				return false;

			auto anotherIt = other.invocationList.begin();
			for (auto it = invocationList.begin(); it != invocationList.end(); ++it)
				if (**it != **anotherIt) return false;

			return true;
		}
		bool operator !=(const multicast_delegate& other) const
		{
			return !(*this == other);
		}

		bool operator ==(const delegate<return_type(parameter_types...)>& other) const
		{
			if (isNull() && other.isNull())
				return true;
			if (other.isNull() || (size() != 1))
				return false;

			return (other.invocation == **invocationList.begin());
		}
		bool operator !=(const delegate<return_type(parameter_types...)>& other) const
		{
			return !(*this == other);
		}

		multicast_delegate& operator +=(const multicast_delegate& other)
		{
			for (auto& item : other.invocationList) // clone, not copy; flattens hierarchy:
				this->invocationList.push_back(new typename delegate_base<return_type(parameter_types...)>::invocation_element(item->object, item->stub));
			return *this;
		}

		template <typename lambda_type>
		multicast_delegate& operator +=(const lambda_type& lambda)
		{
			delegate<return_type(parameter_types...)> d = delegate<return_type(parameter_types...)>::template create<lambda_type>(lambda);
			return *this += d;
		}

		multicast_delegate& operator +=(const delegate<return_type(parameter_types...)>& other)
		{
			if (other.isNull())
				return *this;

			this->invocationList.push_back(new typename delegate_base<return_type(parameter_types...)>::invocation_element(other.invocation.object, other.invocation.stub));
			return *this;
		}

		void operator()(parameter_types... arguments) const
		{
			for (auto& item : invocationList)
				(*(item->stub))(item->object, arguments...);
		}
		void invoke(parameter_types... arguments) const
		{
			for (auto& item : invocationList)
				(*(item->stub))(item->object, arguments...);
		}

		template<typename return_handler>
		void operator()(parameter_types... arguments, return_handler handler) const
		{
			size_t index = 0;
			for (auto& item : invocationList)
			{
				return_type value = (*(item->stub))(item->object, arguments...);
				handler(index, &value);
				++index;
			}
		}
		template<typename return_handler>
		void invoke(parameter_types... arguments, return_handler handler) const
		{
			size_t index = 0;
			for (auto& item : invocationList)
			{
				return_type value = (*(item->stub))(item->object, arguments...);
				handler(index, &value);
				++index;
			}
		}

		void operator()(parameter_types... arguments, delegate<void(size_t, return_type*)> handler) const
		{
			operator() < decltype(handler) > (arguments..., handler);
		}
		void invoke(parameter_types... arguments, delegate<void(size_t, return_type*)> handler) const
		{
			operator() < decltype(handler) > (arguments..., handler);
		}

		void operator()(parameter_types... arguments, std::function<void(size_t, return_type*)> handler) const
		{
			operator() < decltype(handler) > (arguments..., handler);
		}
		void invoke(parameter_types... arguments, std::function<void(size_t, return_type*)> handler) const
		{
			operator() < decltype(handler) > (arguments..., handler);
		}

	private:
		std::vector<typename delegate_base<return_type(parameter_types...)>::invocation_element*> invocationList;
	};
}