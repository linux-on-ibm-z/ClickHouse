#include <gtest/gtest.h>

#include <Common/tests/gtest_global_context.h>
#include <Common/tests/gtest_global_register.h>

#include <Columns/IColumn.h>
#include <DataTypes/DataTypesDecimal.h>
#include <DataTypes/DataTypeFactory.h>
#include <DataTypes/DataTypeLowCardinality.h>
#include <DataTypes/DataTypesNumber.h>
#include <Interpreters/convertFieldToType.h>
#include <Storages/MergeTree/RPNBuilder.h>
#include <Storages/Statistics/Statistics.h>
#include <Storages/StatisticsDescription.h>
#include <Storages/ColumnsDescription.h>
#include <Storages/Statistics/StatisticsTDigest.h>
#include <Storages/Statistics/ConditionSelectivityEstimator.h>
#include <Parsers/parseQuery.h>
#include <Parsers/ExpressionListParsers.h>

using namespace DB;

TEST(Statistics, TDigestLessThan)
{
    /// this is the simplest data which is continuous integeters.
    /// so the estimated errors should be low.

    std::vector<Int64> data;
    data.reserve(100000);
    for (int i = 0; i < 100000; i++)
        data.push_back(i);

    auto test_less_than = [](const std::vector<Int64> & data1,
                             const std::vector<double> & v,
                             const std::vector<double> & answers,
                             const std::vector<double> & eps)
    {

        DB::QuantileTDigest<Int64> t_digest;

        for (Int64 i : data1)
            t_digest.add(i);

        t_digest.compress();

        for (int i = 0; i < v.size(); i ++)
        {
            auto value = v[i];
            auto result = t_digest.getCountLessThan(value);
            auto answer = answers[i];
            auto error = eps[i];
            ASSERT_LE(result, answer * (1 + error));
            ASSERT_GE(result, answer * (1 - error));
        }
    };
    test_less_than(data, {-1, 1e9, 50000.0, 3000.0, 30.0}, {0, 100000, 50000, 3000, 30}, {0, 0, 0.001, 0.001, 0.001});

    std::reverse(data.begin(), data.end());
    test_less_than(data, {-1, 1e9, 50000.0, 3000.0, 30.0}, {0, 100000, 50000, 3000, 30}, {0, 0, 0.001, 0.001, 0.001});
}

TEST(Statistics, CountMinManyDataTypes)
{
    auto test_f = [](const DataTypePtr & data_type, const std::function<void(MutableColumnPtr&, std::size_t)>& insertValue, const String & expression, UInt64 real_result, Float64 eps = 0.001)
    {
        MutableColumnPtr col = data_type->createColumn();
        for (std::size_t i = 0; i < 10000; i++)
        {
            insertValue(col, i);
        }

        auto mock_statistics = [&](const String & column_name)
        {
            ColumnStatisticsDescription mock_description;
            mock_description.data_type = data_type;
            std::vector<StatisticsType> stats_type_to_create({StatisticsType::CountMinSketch});
            for (auto stats_type : stats_type_to_create)
            {
                mock_description.types_to_desc.emplace(stats_type, SingleStatisticsDescription(stats_type, nullptr));
            }
            ColumnDescription column_desc;
            column_desc.name = column_name;
            column_desc.type = data_type;
            column_desc.statistics = mock_description;
            return MergeTreeStatisticsFactory::instance().get(column_desc);
        };
        ColumnStatisticsPtr stats = mock_statistics("col");
        stats->build(std::move(col));

        ConditionSelectivityEstimator estimator;
        estimator.addStatistics(stats);
        estimator.incrementRowCount(10000);

        ParserExpressionWithOptionalAlias exp_parser(false);
        ContextPtr context = getContext().context;
        RPNBuilderTreeContext tree_context(context, Block{{ DataTypeUInt8().createColumnConstWithDefaultValue(1), std::make_shared<DataTypeUInt8>(), "_dummy" }}, {});
        ASTPtr ast = parseQuery(exp_parser, expression, 10000, 10000, 10000);
        RPNBuilderTreeNode node(ast.get(), tree_context);
        auto estimate_result = estimator.estimateRelationProfile(node);
        std::cout << data_type->getName() << " (" << expression << ") " << real_result << " "<< estimate_result.rows << std::endl;
        EXPECT_LT(std::abs(real_result - estimate_result.rows), 10000 * eps);
    };

    ///
    test_f(std::make_shared<DataTypeUInt8>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? UInt8(125) : UInt8(250)); }, "col = 125", 5000);
    test_f(std::make_shared<DataTypeUInt16>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? UInt16(125) : UInt16(250)); }, "col = 125", 5000);
    test_f(std::make_shared<DataTypeUInt32>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? UInt32(125) : UInt32(250)); }, "col = 125", 5000);
    test_f(std::make_shared<DataTypeUInt64>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? UInt64(125) : UInt64(250)); }, "col = 125", 5000);
    test_f(std::make_shared<DataTypeUInt128>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? UInt128(125) : UInt128(250)); }, "col = 125", 5000);
    test_f(std::make_shared<DataTypeUInt256>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? UInt256(125) : UInt256(250)); }, "col = 125", 5000);

    test_f(std::make_shared<DataTypeInt8>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Int8(-125) : Int8(250)); }, "col = -125", 5000);
    test_f(std::make_shared<DataTypeInt16>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Int16(-125) : Int16(250)); }, "col = -125", 5000);
    test_f(std::make_shared<DataTypeInt32>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Int32(-125) : Int32(250)); }, "col = -125", 5000);
    test_f(std::make_shared<DataTypeInt64>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Int64(-125) : Int64(250)); }, "col = -125", 5000);
    test_f(std::make_shared<DataTypeInt128>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Int128(-125) : Int128(250)); }, "col = -125", 5000);
    test_f(std::make_shared<DataTypeInt256>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Int256(-125) : Int256(250)); }, "col = -125", 5000);

    test_f(std::make_shared<DataTypeBFloat16>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? BFloat16(125) : BFloat16(250)); }, "col = 125.0", 5000);
    test_f(std::make_shared<DataTypeBFloat16>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? BFloat16(-125) : BFloat16(250)); }, "col = -125.0", 5000);
    test_f(std::make_shared<DataTypeFloat32>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Float32(125) : Float32(250)); }, "col = 125.0", 5000);
    test_f(std::make_shared<DataTypeFloat32>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Float32(-125) : Float32(250)); }, "col = -125.0", 5000);
    test_f(std::make_shared<DataTypeFloat64>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Float64(125) : Float64(250)); }, "col = 125.0", 5000);
    test_f(std::make_shared<DataTypeFloat64>(), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Float64(-125) : Float64(250)); }, "col = -125.0", 5000);

    test_f(std::make_shared<DataTypeDecimal32>(3, 0), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Decimal32(125) : Decimal32(250)); }, "col = 125", 5000);
    test_f(std::make_shared<DataTypeDecimal64>(3, 0), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Decimal64(125) : Decimal64(250)); }, "col = 125", 5000);
    test_f(std::make_shared<DataTypeDecimal128>(3, 0), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Decimal128(125) : Decimal128(250)); }, "col = 125", 5000);
    test_f(std::make_shared<DataTypeDecimal256>(3, 0), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Decimal256(125) : Decimal256(250)); }, "col = 125", 5000);

    test_f(DataTypeFactory::instance().get("Bool"), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? UInt8(1) : UInt8(0)); }, "col = 1", 5000);
    test_f(std::make_shared<DataTypeDateTime64>(3, "UTC"), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? Decimal64(187618332000) : Decimal64(200000000000)); }, "col = 187618332", 5000);

    test_f(std::make_shared<DataTypeLowCardinality>(std::make_shared<DataTypeUInt32>()), [](MutableColumnPtr & col, std::size_t i) { col->insert(i % 2 == 0 ? UInt32(125) : UInt32(250)); }, "col = 125", 5000);
}

TEST(Statistics, Estimator)
{
    DataTypePtr data_type = std::make_shared<DataTypeInt32>();
    /// column a, distribution 1,2...,10000
    /// column b, distribution 500,600,500,600...
    /// column c, distribution -10000, -1000, -100, -10, -1, 1, 10, 100, 1008, 1009, 1010, ...
    MutableColumnPtr a = DataTypeInt32().createColumn();
    MutableColumnPtr b = DataTypeInt32().createColumn();
    MutableColumnPtr c = DataTypeInt32().createColumn();
    Int32 c_value[] = {-100000, -1000, -100, -10, -1, 1, 10, 100};
    for (Int32 i = 0; i < 10000; i++)
    {
        a->insert(i+1);
        b->insert(i % 2 == 0 ? 500 : 600);
        c->insert(i < 8 ? c_value[i]: 1000+i);
    }

    auto mock_statistics = [&](const String & column_name)
    {
        ColumnStatisticsDescription mock_description;
        mock_description.data_type = data_type;
        std::vector<StatisticsType> stats_type_to_create({StatisticsType::TDigest, /*StatisticsType::Uniq,*/ StatisticsType::CountMinSketch});
        for (auto stats_type : stats_type_to_create)
        {
            mock_description.types_to_desc.emplace(stats_type, SingleStatisticsDescription(stats_type, nullptr));
        }
        ColumnDescription column_desc;
        column_desc.name = column_name;
        column_desc.type = data_type;
        column_desc.statistics = mock_description;
        return MergeTreeStatisticsFactory::instance().get(column_desc);
    };
    ColumnStatisticsPtr stats_a = mock_statistics("a");
    stats_a->build(std::move(a));
    ColumnStatisticsPtr stats_b = mock_statistics("b");
    stats_b->build(std::move(b));
    ColumnStatisticsPtr stats_c = mock_statistics("c");
    stats_c->build(std::move(c));

    ConditionSelectivityEstimator estimator;
    estimator.addStatistics(stats_a);
    estimator.addStatistics(stats_b);
    estimator.addStatistics(stats_c);
    estimator.incrementRowCount(10000);

    auto test_impl = [&](const String & expression, Int32 real_result, Float64 eps)
    {
        ParserExpressionWithOptionalAlias exp_parser(false);
        ContextPtr context = getContext().context;
        RPNBuilderTreeContext tree_context(context, Block{{ DataTypeUInt8().createColumnConstWithDefaultValue(1), std::make_shared<DataTypeUInt8>(), "_dummy" }}, {});
        ASTPtr ast = parseQuery(exp_parser, expression, 10000, 10000, 10000);
        RPNBuilderTreeNode node(ast.get(), tree_context);
        auto estimate_result = estimator.estimateRelationProfile(node);
        std::cout << expression << " " << real_result << " "<< estimate_result.rows << std::endl;
        EXPECT_LT(std::abs(real_result - estimate_result.rows), 10000 * eps);
    };

    auto test_f = [&](const String & expression, Int32 real_result, Float64 eps = 0.001)
    {
        test_impl(expression, real_result, eps);
        /// Let's test 'not expression'
        test_impl("not(" + expression + ")", 10000-real_result, eps);
    };
    ///
    test_f("a in (1,2,3,4,5)", 5);
    test_f("a not in (1,2,3,4,5)", 10000-5);
    test_f("b in (2, 500, 500)", 5000);
    test_f("a < 3 and b = 500", 1);
    test_f("a < 3 and b = 500 and a < b", 1); /// unknown condition 'a < b' assumes 100% selectivity
    test_f("a < 3 or b = 600", 5001);
    test_f("not (a < 3 and b = 500)", 10000-1);
    test_f("c between -1000 and -10", 3);
    test_f("b != 500 and b != 600", 0);
    test_f("not (b != 500 and b != 600)", 10000);
    test_f("b != 500 or b != 600", 10000);
    test_f("not (b != 500 or b != 600)", 0);
    test_f("a < 3 and b != 600", 1);
    test_f("a > 3 and b != 600", 4998);
    test_f("(a > 3 or a < 10) and b != 600", 5000);
    test_f("(a > 3 and a < 10) and b != 600", 3);
    test_f("(a > 3 and a < 10) or (b != 600 and b != 500)", 6);
    test_f("(a > 3 and a < 10) or not (b != 600 and b != 500)", 10000);
    test_f("((a > 3 and a < 10) or (a > 900 and a < 1000) or (a > 9050 and a < 9060))", 114);
    test_f("(a > 3 and a < 1000) or (a > 3 and a < 1011) or (a > 3 and a < 2012)", 2008);
    test_f("(a > 3 and a < 1000) or (a > 3 and a < 1011) or (b = 500)", 5503);
    test_f("(a > 3 and a < 1000) or ((a > 3 and a < 1011) and (b = 500))", 1001, 0.05); /// 5% error
    test_f("((a > 3 and a < 1000) or (a > 3 and a < 1011)) and (b = 500)", 503);
    test_f("a = 5 and a != 6", 1);
}
