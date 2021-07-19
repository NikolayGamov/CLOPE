/// ���������� CLOPE - �������� ������������� ������������ ������


#include <iostream>

#include <vector>
#include <unordered_map>
#include <map>
#include <memory>

#include <algorithm>
#include <cmath>


/// ������ ����������
struct TransactionObject
{
	char m_val; ///< �������� ������� ����������

	/// �����������
	TransactionObject(char val)
		: m_val{ val }
	{}
};


/// ���-������� � ���������� ��� ������������� TransactionObject � �������� �����
namespace std
{
	template<>
	class hash<TransactionObject>
	{
	public:
		std::size_t operator()(const TransactionObject& obj) const noexcept
		{
			return std::hash<decltype(TransactionObject::m_val)>()(obj.m_val);
		}
	};

	template<>
	class equal_to<TransactionObject>
	{
	public:
		bool operator()(const TransactionObject& lhs, const TransactionObject& rhs) const
		{
			return lhs.m_val == rhs.m_val;
		}
	};
}


/// ��������� ��������� �� ������ ����������
using TransObjUPtr = std::unique_ptr<TransactionObject>;
/// ���������� - ����� ��������
using Transaction = std::vector<TransObjUPtr>;
/// ��������� ����������
using Transactions = std::vector<Transaction>;
/// ����� ������ - ������������ ���������� � �� ���������
using DataSet = std::unordered_map<Transaction, int>;


/// �������
class Claster
{
public:
	size_t m_transactionsCount = 0;  ///< ���-�� ����������    N
	size_t m_uniqueObjectsCount = 0; ///< ���������� ��������  W
	size_t m_allObjectsCount = 0;    ///< ����� ��������       S
	std::unordered_map<TransactionObject, size_t> m_countByObjects; ///< ��������� �� ��������  Occ

public:
	/// �������� ����������
	void AddTransaction(const Transaction& tr)
	{
		++m_transactionsCount;
		m_allObjectsCount += tr.size();

		for (auto&& obj : tr)
		{
			_ASSERT(obj);
			++m_countByObjects[*obj];
		}

		m_uniqueObjectsCount = m_countByObjects.size();
	}

	/// ������� ����������
	void RemoveTransaction(const Transaction& tr)
	{
		--m_transactionsCount;
		m_allObjectsCount -= tr.size();

		for (auto&& obj : tr)
		{
			_ASSERT(obj && (m_countByObjects.find(*obj) != m_countByObjects.end()));
			--m_countByObjects[*obj];
		}

		m_uniqueObjectsCount = m_countByObjects.size();
	}

	/// ������ �� �������
	bool IsEmpty() const
	{
		return m_transactionsCount == 0;
	}
};


/// �������� CLOPE
class CLOPE
{
private: // ������� ������
	DataSet& m_dataSet;				 ///< ������ �� �������
	const double m_repulsion;		 ///< ����������� ������������

private: // ���������� ��������������� ������
	std::vector<Claster> m_clasters; ///< ��������� �� ���������

public:
	/// �����������
	CLOPE( DataSet& dataset, double repulsion )
		: m_dataSet{ dataset }
		, m_repulsion{repulsion}
		, m_clasters{}
	{}

public:
	/// ��� 1. �������������
	void Init()
	{
		// ����������� ���������� �� ��������� � ������ ������������� ���������� ��������� � �������� ������ ���������� ����� ��������
		for (auto&& trans : m_dataSet)
			trans.second = AddByMaxProfit(trans.first);
	}

	/// ��� 2. ��������
	void ImproveClustering()
	{
		bool improveMore = true;
		while (improveMore)
		{
			improveMore = false;
			// �������� �� ���� �����������:
			// ���������� �������� �� � ��������, ���������� �� ������ � ������ ������������ ���������
			// ���� ���� �� ���� ���������� �����������, ������ �� ��������� �����
			for (auto&& trans : m_dataSet)
			{
				auto trClasterInd = trans.second;
				m_clasters[trClasterInd].RemoveTransaction(trans.first);
				trans.second = AddByMaxProfit(trans.first);
				auto newTrClasterInd = trans.second;

				if (newTrClasterInd != trClasterInd)
					improveMore = true;
			}

			// ������ ������ �������� �� ������ ��������, ����� �� ��������� �� ��� ��������� �������
			// � ����� ������ ����� �� �������, ��� ��� ��� ����� ��������� �������� ����������������� ������� � �� ��������� �� ������
			m_clasters.erase(std::remove_if(m_clasters.begin(), m_clasters.end(), [](const Claster& cl) { return cl.IsEmpty(); }));
		}
	}

private:
	/// �������� ���������� � ����� �������� �������
	size_t AddByMaxProfit(const Transaction& newTransaction)
	{
		// ����� ���������� ����� �������� �������� �� ��� �������� (������� ������ �����)
		std::map<double, size_t> possibleProfits;

		auto clastersCount = m_clasters.size();

		for (size_t i = 0; i < clastersCount; ++i)
			possibleProfits.emplace(DeltaProfitAdd(m_clasters[i], newTransaction), i);

		possibleProfits.emplace(DeltaProfitAdd(Claster(), newTransaction), clastersCount);

		// ����� �������� ������� - � �������� ��� ���������� ���������� ��������� �����������, � ���� � ������� ����������
		auto bestChoice = *possibleProfits.crbegin();

		if (bestChoice.second == clastersCount)
		{
			m_clasters.push_back(Claster());
		}

		m_clasters[bestChoice.second].AddTransaction(newTransaction);

		// ����� ������ ��������, � ������� �������� ����������
		return bestChoice.second;
	}

	/// ���������� ��������� �������� ��� ���������� � ���� ����������
	double DeltaProfitAdd(const Claster& claster, const Transaction& transaction)
	{
		// ���������� ��������� �������� ��� ���������� � ���� ���������� = ��������� ����� - ��������� ��
		auto newClaster = claster;
		newClaster.AddTransaction(transaction);

		return Profit(newClaster) - Profit(claster);
	}

	/// ��������� ��������
	double Profit(const Claster& cl)
	{

		if (cl.IsEmpty)
			return 0.0;

		// ��������� �������� = S x N / ( W ^ r )
		double numerator = cl.m_allObjectsCount * cl.m_transactionsCount / std::pow(cl.m_uniqueObjectsCount, m_repulsion);
		size_t divider = cl.m_transactionsCount;

		_ASSERT(divider != 0);

		return numerator / divider;
	}
};


/// �������� ����-���� �� ������ ����������. ��������� ������ ���������� ������ �������� � ��������� ���������
std::unique_ptr<DataSet> CreateDataSet(const Transactions& transactions)
{
	constexpr int invalidClasterIndex = -1;

	auto ds = std::make_unique<DataSet>();
	for (auto&& tr : transactions)
		ds->emplace(tr, invalidClasterIndex);

	return ds;
}


/// �������� ������� ������
Transactions c_testTransactions{/*����� ����������*/};
/// �������� ����������� ������������
double c_repulsion{ 2.0 };


/// ����� �����
void main()
{
	// ������� �������
	auto dataSet = CreateDataSet(c_testTransactions);

	// ������� CLOPE-��������������
	auto clope = std::make_unique<CLOPE>(*dataSet, c_repulsion);

	// ��� 1. �������������
	clope->Init();
	// ��� 2. ��������
	clope->ImproveClustering();
}