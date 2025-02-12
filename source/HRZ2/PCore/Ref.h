#pragma once

namespace HRZ2
{
	template<typename T>
	class Ref final
	{
	private:
		T *m_Ref = nullptr;

	public:
		Ref() = default;

		Ref(const Ref<T>& Other)
		{
			Assign(Other.m_Ref);
		}

		Ref(Ref<T>&& Other) noexcept
		{
			m_Ref = std::exchange(Other.m_Ref, nullptr);
		}

		Ref(T *Other)
		{
			Assign(Other);
		}

		~Ref()
		{
			Assign(nullptr);
		}

		Ref<T>& operator=(const Ref<T>& Other)
		{
			Assign(Other.m_Ref);
			return *this;
		}

		Ref<T>& operator=(Ref<T>&& Other) noexcept
		{
			m_Ref = std::exchange(Other.m_Ref, nullptr);
			return *this;
		}

		Ref<T>& operator=(T *Other)
		{
			Assign(Other);
			return *this;
		}

		T *GetPtr() const
		{
			return m_Ref;
		}

		T *operator->() const
		{
			return m_Ref;
		}

		operator T *() const
		{
			return m_Ref;
		}

		explicit operator bool() const
		{
			return m_Ref != nullptr;
		}

	private:
		void Assign(T *Other)
		{
			if (m_Ref != Other)
			{
				if (Other)
					Other->AddRef();

				const auto old = std::exchange(m_Ref, Other);

				if (old)
					old->Release();
			}
		}
	};
}
