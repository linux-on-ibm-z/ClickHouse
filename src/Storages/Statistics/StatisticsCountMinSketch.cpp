#include <Storages/Statistics/StatisticsCountMinSketch.h>
#include <Common/FieldVisitors.h>
#include <DataTypes/DataTypeLowCardinality.h>
#include <DataTypes/DataTypeNullable.h>
#include <IO/ReadHelpers.h>
#include <IO/WriteHelpers.h>
#include <Interpreters/convertFieldToType.h>
#include <base/bit_cast.h>

#if USE_DATASKETCHES

namespace DB
{

template <typename To>
class FieldVisitorBitCast : public StaticVisitor<To>
{
public:
    template <typename From>
    To operator() (DecimalField<From> & x) const { return bit_cast<To>(x.getValue()); }

    template <typename From>
    To operator()(const From & x) const { return bit_cast<To>(x); }
};

namespace ErrorCodes
{
extern const int LOGICAL_ERROR;
extern const int ILLEGAL_STATISTICS;
}

/// Constants chosen based on rolling dices.
/// The values provides:
///     1. an error tolerance of 0.1% (ε = 0.001)
///     2. a confidence level of 99.9% (δ = 0.001).
/// And sketch the size is 152kb.
static constexpr auto num_hashes = 7uz;
static constexpr auto num_buckets = 2718uz;

StatisticsCountMinSketch::StatisticsCountMinSketch(const SingleStatisticsDescription & description, const DataTypePtr & data_type_)
    : IStatistics(description)
    , sketch(num_hashes, num_buckets)
    , data_type(removeNullable(data_type_))
{
}

Float64 StatisticsCountMinSketch::estimateEqual(const Field & val) const
{
    /// Try to convert field to data_type. Converting string to proper data types such as: number, date, datetime, IPv4, Decimal etc.
    /// Return null if val larger than the range of data_type
    ///
    /// For example: if data_type is Int32:
    ///     1. For 1.0, 1, '1', return Field(1)
    ///     2. For 1.1, max_value_int64, return null
    Field val_converted = convertFieldToType(val, *data_type);
    if (val_converted.isNull())
        return 0;

    if (data_type->isValueRepresentedByNumber())
    {
        if (val_converted.getType() == Field::Types::Float64)
        {
            auto getEstimate = [&s = sketch, &val_converted]<typename To>() {
                To v = To(val_converted.safeGet<Float64>());
                return s.get_estimate(&v, sizeof(To));
            };

            if (data_type->getColumnType() == TypeIndex::BFloat16)
                return getEstimate.template operator()<BFloat16>();
            if (data_type->getColumnType() == TypeIndex::Float32)
                return getEstimate.template operator()<Float32>();
        }
        if constexpr (std::endian::native == std::endian::big)
        {
            auto getEstimate = [&s = sketch, &val_converted]<typename To>(const FieldVisitorBitCast<To> visitor) {
                To v = applyVisitor(visitor, val_converted);
                return s.get_estimate(&v, sizeof(To));
            };

            size_t memSize = data_type->getSizeOfValueInMemory();
            if (memSize == sizeof(UInt256))
                return getEstimate(FieldVisitorBitCast<UInt256>());
            if (memSize == sizeof(UInt128))
                return getEstimate(FieldVisitorBitCast<UInt128>());
            if (memSize == sizeof(UInt64))
                return getEstimate(FieldVisitorBitCast<UInt64>());
            if (memSize == sizeof(UInt32))
                return getEstimate(FieldVisitorBitCast<UInt32>());
            if (memSize == sizeof(UInt16))
                return getEstimate(FieldVisitorBitCast<UInt16>());
            if (memSize == sizeof(UInt8))
                return getEstimate(FieldVisitorBitCast<UInt8>());

            throw Exception(ErrorCodes::LOGICAL_ERROR, "Statistics 'countmin' does not support estimate data type of {} on big endian platforms", data_type->getName());
        } else
            return sketch.get_estimate(&val_converted, data_type->getSizeOfValueInMemory());
    }

    if (isStringOrFixedString(data_type))
        return sketch.get_estimate(val.safeGet<String>());

    throw Exception(ErrorCodes::LOGICAL_ERROR, "Statistics 'countmin' does not support estimate data type of {}", data_type->getName());
}

void StatisticsCountMinSketch::build(const ColumnPtr & column)
{
    for (size_t row = 0; row < column->size(); ++row)
    {
        if (column->isNullAt(row))
            continue;
        auto data = column->getDataAt(row);
        sketch.update(data.data, data.size, 1);
    }
}

void StatisticsCountMinSketch::merge(const StatisticsPtr & other_stats)
{
    const StatisticsCountMinSketch * other = typeid_cast<const StatisticsCountMinSketch *>(other_stats.get());
    sketch.merge(other->sketch);
}

void StatisticsCountMinSketch::serialize(WriteBuffer & buf)
{
    Sketch::vector_bytes bytes = sketch.serialize();
    writeIntBinary(static_cast<UInt64>(bytes.size()), buf);
    buf.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

void StatisticsCountMinSketch::deserialize(ReadBuffer & buf)
{
    UInt64 size;
    readIntBinary(size, buf);

    Sketch::vector_bytes bytes;
    bytes.resize(size); /// To avoid 'container-overflow' in AddressSanitizer checking
    buf.readStrict(reinterpret_cast<char *>(bytes.data()), size);

    sketch = Sketch::deserialize(bytes.data(), size);
}


void countMinSketchStatisticsValidator(const SingleStatisticsDescription & /*description*/, const DataTypePtr & data_type)
{
    DataTypePtr inner_data_type = removeNullable(data_type);
    inner_data_type = removeLowCardinalityAndNullable(inner_data_type);
    if (!inner_data_type->isValueRepresentedByNumber() && !isStringOrFixedString(inner_data_type))
        throw Exception(ErrorCodes::ILLEGAL_STATISTICS, "Statistics of type 'countmin' does not support type {}", data_type->getName());
}

StatisticsPtr countMinSketchStatisticsCreator(const SingleStatisticsDescription & description, const DataTypePtr & data_type)
{
    return std::make_shared<StatisticsCountMinSketch>(description, data_type);
}

}

#endif
