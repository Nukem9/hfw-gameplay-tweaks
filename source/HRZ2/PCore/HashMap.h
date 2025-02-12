#pragma once

namespace HRZ2
{
	template<typename TKey, typename TValue>
	class HashMapBase
	{
	public:
		template<bool Const, typename PtrType>
		class internal_iterator;

		using key_type = TKey;
		using value_type = TKey;
		using size_type = size_t;
		using difference_type = ptrdiff_t;
		//using hasher = Hash;

		using reference = value_type&;
		using const_reference = const value_type&;

		using pointer = void;
		using const_pointer = void;

		struct KeyValue
		{
			const TKey Key;
			TValue Value;
		};

	private:
		struct Assoc // sizeof(Assoc<String, String>) = 0x18. Hash at 0x10.
		{
			KeyValue m_KeyValue;
			int m_Hash = 0;
		};
		static_assert(sizeof(Assoc) == 32);

		Assoc *m_DataPtr = nullptr;
		int m_NumKeys = 0;
		int m_Capacity = 0;

	public:
		using iterator = internal_iterator<false, Assoc *>;
		using const_iterator = internal_iterator<true, const Assoc *>;

		template<bool Const, typename PtrType>
		class internal_iterator
		{
		private:
			PtrType m_Current = nullptr;
			PtrType m_End = nullptr;

		public:
			internal_iterator() = delete;

			explicit internal_iterator(PtrType Table, int Start, int End)
			{
				if (Table)
				{
					m_Current = &Table[Start];
					m_End = &Table[End];

					SkipEmptyElements();
				}
			}

			internal_iterator& operator++()
			{
				m_Current++;
				SkipEmptyElements();

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
			KeyValue& operator*()
			{
				return m_Current->m_KeyValue;
			}

			const KeyValue& operator*() const
			{
				return m_Current->m_KeyValue;
			}

		private:
			void SkipEmptyElements()
			{
				while (!AtEnd() && m_Current->m_Hash == 0)
					m_Current++;
			}

			bool AtEnd() const
			{
				return m_Current == m_End;
			}
		};

		HashMapBase() = default;
		HashMapBase(const HashMapBase&) = delete;
		~HashMapBase() = delete;

		iterator begin()
		{
			return iterator(m_DataPtr, 0, m_Capacity);
		}

		iterator end()
		{
			return iterator(m_DataPtr, m_Capacity, m_Capacity);
		}

		const_iterator begin() const
		{
			return const_iterator(m_DataPtr, 0, m_Capacity);
		}

		const_iterator end() const
		{
			return const_iterator(m_DataPtr, m_Capacity, m_Capacity);
		}

		HashMapBase& operator=(const HashMapBase&) = delete;
	};

	template<typename TKey, typename TValue>
	class HashMap final : public HashMapBase<TKey, TValue>
	{
	public:
		HashMap() = default;
		HashMap(const HashMap&) = delete;
		~HashMap() = delete;
	};
}
