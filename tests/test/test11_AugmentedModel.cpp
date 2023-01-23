#include <gtest/gtest.h>
#include <UtilsQt/AugmentedModel.h>
#include <QSignalSpy>
#include <QAbstractListModel>
#include <QVariantList>
#include <QList>
#include <algorithm>

namespace {
class TestModel : public QAbstractListModel
{
    //Q_OBJECT
public:
    enum Roles {
        Name = Qt::DisplayRole,
        Male = Qt::UserRole,
        Age,
    };

    TestModel(QObject* parent = nullptr): QAbstractListModel(parent) { }
    ~TestModel() override { }

    static TestModel& instance() {
        static TestModel model;
        static bool initialized = false;

        if (!initialized) {
            model.reset();
            initialized = true;
        }

        return model;
    }

    void reset() {
        beginResetModel();
        m_data = {
            {"Ivan", true, 23},
            {"Dimon", true, 31},
            {"Olga", false, 20}
        };
        endResetModel();
    }

    int rowCount(const QModelIndex& /*parent*/) const override { return m_data.size(); }

    QVariant data(const QModelIndex& index, int role) const override {
        assert(index.column() == 0);
        assert(m_roleIndexRemap.contains(role));
        assert(index.row() >= 0 && index.row() < rowCount({}));
        return m_data.at(index.row()).at(m_roleIndexRemap.value(role));
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
            {Roles::Name, "display"},
            {Roles::Male, "male"},
            {Roles::Age, "age"}
        };
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role) override {
        assert(index.column() == 0);
        changeData(index.row(), role, value);
        return true;
    }

    void changeData(int row, const QVector<int>& roles, const QVariantList& newData) {
        assert(row >= 0 && row < m_data.size());
        assert(roles.size() == newData.size());
        assert(roles.size() > 0);

        for (auto role : roles)
            assert(m_roleIndexRemap.contains(role));

        for (int i = 0; i < roles.size(); i++)
            m_data[row][m_roleIndexRemap.value(roles.at(i))] = newData.at(i);

        emit dataChanged(index(row), index(row), roles);
    }

    void changeData(int row, int role, const QVariant& newData) {
        changeData(row, QVector<int>{role}, QVariantList{newData});
    }

    void addRow(const QVariantList& values) {
        assert(values.size() == m_roleIndexRemap.size());
        beginInsertRows({}, m_data.size(), m_data.size());
        m_data.append(values);
        endInsertRows();
    }

private:
    QList<QVariantList> m_data;

    const QHash<int,int> m_roleIndexRemap = {
        {Roles::Name, 0},
        {Roles::Male, 1},
        {Roles::Age, 2},
    };
};

QList<QVariantList> extractData(const QAbstractItemModel* model) {
    assert(model);
    assert(model->columnCount() == 1);

    int size = model->rowCount();
    QList<QVariantList> data;
    QList<int> roles = model->roleNames().keys();
    std::sort(roles.begin(), roles.end());

    for (int row = 0; row < size; row++) {
        QVariantList line;

        for (int role = 0; role < roles.size(); role++)
            line.append(model->data(model->index(row, 0), roles.at(role)));

        data.append(line);
    }

    return data;
}

} // namespace

TEST(UtilsQt, AugmentedModel_InitialTest)
{
    auto extractedTestData = extractData(&TestModel::instance());
    QList<QVariantList> correctTestData {
        {"Ivan", true, 23},
        {"Dimon", true, 31},
        {"Olga", false, 20}
    };
    ASSERT_EQ(extractedTestData, correctTestData);

    QSignalSpy spyDataChanged(&TestModel::instance(), &TestModel::dataChanged);
    TestModel::instance().setData(TestModel::instance().index(1), 32, TestModel::Roles::Age);
    correctTestData[1][2] = 32;
    extractedTestData = extractData(&TestModel::instance());
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyDataChanged.count(), 1);
    ASSERT_EQ(spyDataChanged.last()[0].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[1].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().size(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(0), TestModel::Roles::Age);

    QSignalSpy spyInserted(&TestModel::instance(), &TestModel::rowsInserted);
    TestModel::instance().addRow({"Sergii", true, 40});
    correctTestData.append(QVariantList{"Sergii", true, 40});
    extractedTestData = extractData(&TestModel::instance());
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyInserted.count(), 1);
}

TEST(UtilsQt, AugmentedModel_Transparent_Without_Calculated)
{
    TestModel::instance().reset();

    AugmentedModel model;
    model.setSourceModel(&TestModel::instance());

    auto extractedTestData = extractData(&model);
    QList<QVariantList> correctTestData {
        {"Ivan", true, 23},
        {"Dimon", true, 31},
        {"Olga", false, 20}
    };
    ASSERT_EQ(extractedTestData, correctTestData);

    QSignalSpy spyDataChanged(&model, &TestModel::dataChanged);
    TestModel::instance().setData(model.index(1, 0, {}), 32, TestModel::Roles::Age);
    correctTestData[1][2] = 32;
    extractedTestData = extractData(&model);
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyDataChanged.count(), 1);
    ASSERT_EQ(spyDataChanged.last()[0].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[1].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().size(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(0), TestModel::Roles::Age);

    QSignalSpy spyInserted(&model, &TestModel::rowsInserted);
    TestModel::instance().addRow({"Sergii", true, 40});
    correctTestData.append(QVariantList{"Sergii", true, 40});
    extractedTestData = extractData(&model);
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyInserted.count(), 1);

}

TEST(UtilsQt, AugmentedModel_Calculated)
{
    TestModel::instance().reset();

    AugmentedModel model;
    auto updater = model.addCalculatedRole("increasedAge", {"age"}, [](const QVariantList& src) -> QVariant {
        assert(src.size() == 1);
        return src[0].toInt() + 1;
    });
    model.setSourceModel(&TestModel::instance());

    auto extractedTestData = extractData(&model);
    QList<QVariantList> correctTestData {
        {"Ivan", true, 23, 24},
        {"Dimon", true, 31, 32},
        {"Olga", false, 20, 21}
    };
    ASSERT_EQ(extractedTestData, correctTestData);

    // Source change
    QSignalSpy spyDataChanged(&model, &TestModel::dataChanged);
    TestModel::instance().setData(model.index(1, 0, {}), 40, TestModel::Roles::Age);
    correctTestData[1][2] = 40;
    correctTestData[1][3] = 41;
    extractedTestData = extractData(&model);
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyDataChanged.count(), 1);
    ASSERT_EQ(spyDataChanged.last()[0].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[1].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().size(), 2);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(0), TestModel::Roles::Age);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(1), TestModel::Roles::Age + 1);

    TestModel::instance().setData(model.index(1, 0, {}), "Ivan_", TestModel::Roles::Name);
    correctTestData[1][0] = "Ivan_";
    extractedTestData = extractData(&model);
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyDataChanged.count(), 2);
    ASSERT_EQ(spyDataChanged.last()[0].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[1].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().size(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(0), TestModel::Roles::Name);

    // Insertion
    QSignalSpy spyInserted(&model, &TestModel::rowsInserted);
    TestModel::instance().addRow({"Sergii", true, 40});
    correctTestData.append(QVariantList{"Sergii", true, 40, 41});
    extractedTestData = extractData(&model);
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyInserted.count(), 1);

    // Updater
    updater(model.index(0, 0, {}), model.index(1, 0, {}));
    ASSERT_EQ(spyDataChanged.count(), 3);
    ASSERT_EQ(spyDataChanged.last()[0].toModelIndex().row(), 0);
    ASSERT_EQ(spyDataChanged.last()[1].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().size(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(0), TestModel::Roles::Age + 1);

    // Update all
    model.updateAllCalculatedRoles();
    ASSERT_EQ(spyDataChanged.count(), 4);
    ASSERT_EQ(spyDataChanged.last()[0].toModelIndex().row(), 0);
    ASSERT_EQ(spyDataChanged.last()[1].toModelIndex().row(), 3);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().size(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(0), TestModel::Roles::Age + 1);
}

TEST(UtilsQt, AugmentedModel_Calculated_2)
{
    AugmentedModel model;
    model.addCalculatedRole("increasedAge", {"age", TestModel::Roles::Male}, [](const QVariantList& src) -> QVariant {
        assert(src.size() == 2);
        const auto age = src[0].toInt();
        const auto isMale = src[1].toBool();
        return isMale ? age : age + 1;
    });
    model.addCalculatedRole("name2", {TestModel::Roles::Name}, [](const QVariantList& src) -> QVariant {
        assert(src.size() == 1);
        return src[0].toString() + "2";
    });
    model.setSourceModel(&TestModel::instance());

    TestModel::instance().reset();

    auto extractedTestData = extractData(&model);
    QList<QVariantList> correctTestData {
        {"Ivan", true, 23, 23, "Ivan2"},
        {"Dimon", true, 31, 31, "Dimon2"},
        {"Olga", false, 20, 21, "Olga2"}
    };
    ASSERT_EQ(extractedTestData, correctTestData);

    // Source change
    QSignalSpy spyDataChanged(&model, &TestModel::dataChanged);
    TestModel::instance().setData(model.index(1, 0, {}), 40, TestModel::Roles::Age);
    correctTestData[1][2] = 40;
    correctTestData[1][3] = 40;
    extractedTestData = extractData(&model);
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyDataChanged.count(), 1);
    ASSERT_EQ(spyDataChanged.last()[0].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[1].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().size(), 2);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(0), TestModel::Roles::Age);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(1), TestModel::Roles::Age + 1);

    TestModel::instance().setData(model.index(1, 0, {}), false, TestModel::Roles::Male);
    correctTestData[1][1] = false;
    correctTestData[1][2] = 40;
    correctTestData[1][3] = 41;
    extractedTestData = extractData(&model);
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyDataChanged.count(), 2);
    ASSERT_EQ(spyDataChanged.last()[0].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[1].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().size(), 2);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(0), TestModel::Roles::Male);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(1), TestModel::Roles::Age + 1);

    TestModel::instance().setData(model.index(1, 0, {}), "Nastya", TestModel::Roles::Name);
    correctTestData[1][0] = "Nastya";
    correctTestData[1][4] = "Nastya2";
    extractedTestData = extractData(&model);
    ASSERT_EQ(extractedTestData, correctTestData);
    ASSERT_EQ(spyDataChanged.count(), 3);
    ASSERT_EQ(spyDataChanged.last()[0].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[1].toModelIndex().row(), 1);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().size(), 2);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(0), TestModel::Roles::Name);
    ASSERT_EQ(spyDataChanged.last()[2].value<QVector<int>>().at(1), TestModel::Roles::Age + 2);
}
