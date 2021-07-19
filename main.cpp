/// Реализация CLOPE - алгоритм кластеризации категорийных данных


#include <iostream>

#include <vector>
#include <unordered_map>
#include <map>
#include <memory>

#include <algorithm>
#include <cmath>


/// Объект транзакции
struct TransactionObject
{
	char m_val; ///< свойство объекта транзакции

	/// Конструктор
	TransactionObject(char val)
		: m_val{ val }
	{}
};


/// Хэш-функция и компаратор для использования TransactionObject в качестве ключа
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


/// Владеющий указатель на объект транзакции
using TransObjUPtr = std::unique_ptr<TransactionObject>;
/// Транзакция - набор объектов
using Transaction = std::vector<TransObjUPtr>;
/// Множество транзакций
using Transactions = std::vector<Transaction>;
/// Набор данных - соответствие транзакций и их кластеров
using DataSet = std::unordered_map<Transaction, int>;


/// Кластер
class Claster
{
public:
	size_t m_transactionsCount = 0;  ///< Кол-во транзакций    N
	size_t m_uniqueObjectsCount = 0; ///< Уникальных объектов  W
	size_t m_allObjectsCount = 0;    ///< Всего объектов       S
	std::unordered_map<TransactionObject, size_t> m_countByObjects; ///< Вхождений по объектам  Occ

public:
	/// Добавить транзакцию
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

	/// Удалить транзакцию
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

	/// Пустой ли кластер
	bool IsEmpty() const
	{
		return m_transactionsCount == 0;
	}
};


/// Алгоритм CLOPE
class CLOPE
{
private: // Входные данные
	DataSet& m_dataSet;				 ///< Ссылка на датасет
	const double m_repulsion;		 ///< Коэффициент отталкивания

private: // Внутренние вспомогательные данные
	std::vector<Claster> m_clasters; ///< Разбиение по кластерам

public:
	/// Конструктор
	CLOPE( DataSet& dataset, double repulsion )
		: m_dataSet{ dataset }
		, m_repulsion{repulsion}
		, m_clasters{}
	{}

public:
	/// Шаг 1. Инициализация
	void Init()
	{
		// распределим транзакции по кластерам с учетом максимального приращения стоимости и припишем каждой транзакции номер кластера
		for (auto&& trans : m_dataSet)
			trans.second = AddByMaxProfit(trans.first);
	}

	/// Шаг 2. Итерация
	void ImproveClustering()
	{
		bool improveMore = true;
		while (improveMore)
		{
			improveMore = false;
			// проходим по всем транзакциям:
			// транзакцию вынимаем из её кластера, записываем ее заново с учётом максимизации стоимости
			// если хотя бы одну транзакцию переместили, пойдем на следующий заход
			for (auto&& trans : m_dataSet)
			{
				auto trClasterInd = trans.second;
				m_clasters[trClasterInd].RemoveTransaction(trans.first);
				trans.second = AddByMaxProfit(trans.first);
				auto newTrClasterInd = trans.second;

				if (newTrClasterInd != trClasterInd)
					improveMore = true;
			}

			// удалим пустые кластеры на каждой итерации, чтобы не учитывать их при следующем проходе
			// в конце пустые можно не чистить, так как сам набор кластеров является всмпомогательными данными и на разбиение не влияет
			m_clasters.erase(std::remove_if(m_clasters.begin(), m_clasters.end(), [](const Claster& cl) { return cl.IsEmpty(); }));
		}
	}

private:
	/// Добавить транзакцию в самый выгодный кластер
	size_t AddByMaxProfit(const Transaction& newTransaction)
	{
		// новую транзакцию будем пытаться записать во все кластеры (включая пустой новый)
		std::map<double, size_t> possibleProfits;

		auto clastersCount = m_clasters.size();

		for (size_t i = 0; i < clastersCount; ++i)
			possibleProfits.emplace(DeltaProfitAdd(m_clasters[i], newTransaction), i);

		possibleProfits.emplace(DeltaProfitAdd(Claster(), newTransaction), clastersCount);

		// самый выгодный кластер - у которого при добавление приращение стоимости максимально, в него и положим транзакцию
		auto bestChoice = *possibleProfits.crbegin();

		if (bestChoice.second == clastersCount)
		{
			m_clasters.push_back(Claster());
		}

		m_clasters[bestChoice.second].AddTransaction(newTransaction);

		// вернём индекс кластера, в который положили транзакцию
		return bestChoice.second;
	}

	/// Приращение стоимости кластера при добавлении в него транзакции
	double DeltaProfitAdd(const Claster& claster, const Transaction& transaction)
	{
		// приращение стоимости кластера при добавлении в него транзакции = стоимость после - стоимость до
		auto newClaster = claster;
		newClaster.AddTransaction(transaction);

		return Profit(newClaster) - Profit(claster);
	}

	/// Стоимость кластера
	double Profit(const Claster& cl)
	{

		if (cl.IsEmpty)
			return 0.0;

		// стоимость кластера = S x N / ( W ^ r )
		double numerator = cl.m_allObjectsCount * cl.m_transactionsCount / std::pow(cl.m_uniqueObjectsCount, m_repulsion);
		size_t divider = cl.m_transactionsCount;

		_ASSERT(divider != 0);

		return numerator / divider;
	}
};


/// Создание дата-сета из набора транзакций. Добавляет каждой транзакции индекс кластера и отсеивает дубликаты
std::unique_ptr<DataSet> CreateDataSet(const Transactions& transactions)
{
	constexpr int invalidClasterIndex = -1;

	auto ds = std::make_unique<DataSet>();
	for (auto&& tr : transactions)
		ds->emplace(tr, invalidClasterIndex);

	return ds;
}


/// Тестовые входные данные
Transactions c_testTransactions{/*набор транзакций*/};
/// Тестовый коэффициент отталкивания
double c_repulsion{ 2.0 };


/// Точка входа
void main()
{
	// создаем датасет
	auto dataSet = CreateDataSet(c_testTransactions);

	// создаем CLOPE-распределитель
	auto clope = std::make_unique<CLOPE>(*dataSet, c_repulsion);

	// Шаг 1. инициализация
	clope->Init();
	// Шаг 2. итерация
	clope->ImproveClustering();
}