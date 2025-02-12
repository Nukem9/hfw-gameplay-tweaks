#pragma once

namespace HRZ2
{
	class ArrayBase
	{
	public:
		using size_type = size_t;

	protected:
		int m_Length = 0;
		int m_Max = 0;

	public:
		bool empty() const
		{
			return m_Length == 0;
		}

		size_type size() const
		{
			return m_Length;
		}

		size_t capacity() const
		{
			return m_Max;
		}
	};

	template<typename T>
	class Array final : public ArrayBase
	{
	public:
		template<bool Const, typename PtrType = std::conditional_t<Const, const_pointer, pointer>>
		class internal_iterator;

		using value_type = T;
		using difference_type = ptrdiff_t;

		using reference = value_type&;
		using const_reference = const value_type&;

		using pointer = value_type *;
		using const_pointer = const value_type *;

		using iterator = internal_iterator<false>;
		using const_iterator = internal_iterator<true>;

	private:
		T *m_Data = nullptr;

	public:
		template<bool Const, typename PtrType>
		class internal_iterator
		{
		private:
			PtrType m_Current = nullptr;

		public:
			internal_iterator() = delete;

			explicit internal_iterator(PtrType Current) : m_Current(Current) {}

			internal_iterator& operator++()
			{
				m_Current++;
				return *this;
			}

			bool operator==(const internal_iterator& Other) const
			{
				return m_Current == Other.m_Current;
			}

			bool operator!=(const internal_iterator& Other) const
			{
				return m_Current != Other.m_Current;
			}

			template<typename = void>
			requires(!Const)
			reference operator*()
			{
				return *m_Current;
			}

			const_reference operator*() const
			{
				return *m_Current;
			}
		};

		Array() = default;

		Array(const Array<T>& Other)
		{
			const auto newLength = Other.m_Length;

			if (newLength > 0)
			{
				m_Data = AllocateEntries(newLength);
				std::uninitialized_copy_n(Other.m_Data, newLength, m_Data);
			}

			m_Length = newLength;
			m_Max = newLength;
		}

		Array(Array<T>&& Other) noexcept
		{
			m_Data = std::exchange(Other.m_Data, nullptr);
			m_Length = std::exchange(Other.m_Length, 0);
			m_Max = std::exchange(Other.m_Max, 0);
		}

		~Array()
		{
			ClearAndFree();
		}

		Array<T>& operator=(const Array<T>& Other)
		{
			if (this == &Other)
				return *this;

			const auto newLength = Other.m_Length;
			T *newEntries = nullptr;

			ClearAndFree();

			if (newLength > 0)
			{
				newEntries = AllocateEntries(newLength);
				std::uninitialized_copy_n(Other.m_Data, newLength, newEntries);
			}

			m_Data = newEntries;
			m_Length = newLength;
			m_Max = newLength;

			return *this;
		}

		Array<T>& operator=(Array<T>&& Other) noexcept
		{
			if (this == &Other)
				return *this;

			ClearAndFree();
			m_Data = std::exchange(Other.m_Data, nullptr);
			m_Length = std::exchange(Other.m_Length, 0);
			m_Max = std::exchange(Other.m_Max, 0);

			return *this;
		}

		reference operator[](size_type Pos)
		{
			return m_Data[Pos];
		}

		const_reference operator[](size_type Pos) const
		{
			return m_Data[Pos];
		}

		reference front()
		{
			return m_Data[0];
		}

		const_reference front() const
		{
			return m_Data[0];
		}

		reference back()
		{
			return m_Data[m_Length - 1];
		}

		const_reference back() const
		{
			return m_Data[m_Length - 1];
		}

		T *data()
		{
			return m_Data;
		}

		const T *data() const
		{
			return m_Data;
		}

		iterator begin()
		{
			return iterator(&m_Data[0]);
		}

		iterator end()
		{
			return iterator(&m_Data[m_Length]);
		}

		iterator begin() const
		{
			return iterator(&m_Data[0]);
		}

		iterator end() const
		{
			return iterator(&m_Data[m_Length]);
		}

		void clear()
		{
			std::destroy_n(m_Data, m_Length);
			m_Length = 0;
		}

		void push_back(const T& Value)
		{
			if (m_Length == m_Max)
			{
				const auto oldLength = m_Length;
				const auto newMaxLength = m_Max == 0 ? 1 : m_Max * 2;

				auto newEntries = AllocateEntries(newMaxLength);
				std::uninitialized_copy_n(m_Data, m_Length, newEntries);
				ClearAndFree();

				m_Data = newEntries;
				m_Length = oldLength;
				m_Max = newMaxLength;
			}

			std::construct_at(&m_Data[m_Length++], Value);
		}

		void pop_back()
		{
			std::destroy_at(&m_Data[--m_Length]);
		}

	private:
		void ClearAndFree()
		{
			const auto func = Offsets::Signature("48 8B D1 48 85 C9 74 67 48 3B 0D")
				.ToPointer<void(void *)>();

			clear();
			func(m_Data); // nullptr is allowed
		}

		static T *AllocateEntries(size_type Count)
		{
			const auto func = Offsets::Signature(
								  "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 65 48 8B 04 25 58 00 00 00 48 8B FA 48 8B F1 48 8B 18")
								  .ToPointer<void *(size_t, size_t)>();

			return static_cast<T *>(func(Count * sizeof(T), alignof(T)));
		}
	};
}
