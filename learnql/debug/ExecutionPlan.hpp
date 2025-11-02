#ifndef LEARNQL_DEBUG_EXECUTION_PLAN_HPP
#define LEARNQL_DEBUG_EXECUTION_PLAN_HPP

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>

namespace learnql::debug {

/**
 * @brief Type of operation in the execution plan
 */
enum class OperationType {
    TableScan,      // Full table scan
    IndexScan,      // Index-based scan
    Filter,         // Filter operation
    Join,           // Join operation
    GroupBy,        // Group-by aggregation
    Sort,           // Sorting operation
    Limit,          // Limit operation
    Projection      // Projection (select specific fields)
};

/**
 * @brief Convert operation type to string
 */
inline std::string operation_type_to_string(OperationType type) {
    switch (type) {
        case OperationType::TableScan: return "TABLE_SCAN";
        case OperationType::IndexScan: return "INDEX_SCAN";
        case OperationType::Filter: return "FILTER";
        case OperationType::Join: return "JOIN";
        case OperationType::GroupBy: return "GROUP_BY";
        case OperationType::Sort: return "SORT";
        case OperationType::Limit: return "LIMIT";
        case OperationType::Projection: return "PROJECTION";
    }
    return "UNKNOWN";
}

/**
 * @brief Node in the execution plan tree
 *
 * Represents a single operation in the query execution plan.
 */
class ExecutionPlanNode {
public:
    /**
     * @brief Constructs an execution plan node
     * @param type Operation type
     * @param description Human-readable description
     */
    ExecutionPlanNode(OperationType type, std::string description)
        : type_(type), description_(std::move(description)),
          estimated_rows_(0), estimated_cost_(0.0) {}

    /**
     * @brief Sets estimated rows
     */
    void set_estimated_rows(std::size_t rows) {
        estimated_rows_ = rows;
    }

    /**
     * @brief Sets estimated cost
     */
    void set_estimated_cost(double cost) {
        estimated_cost_ = cost;
    }

    /**
     * @brief Adds a child node
     */
    void add_child(std::shared_ptr<ExecutionPlanNode> child) {
        children_.push_back(std::move(child));
    }

    /**
     * @brief Gets operation type
     */
    [[nodiscard]] OperationType type() const noexcept {
        return type_;
    }

    /**
     * @brief Gets description
     */
    [[nodiscard]] const std::string& description() const noexcept {
        return description_;
    }

    /**
     * @brief Gets children
     */
    [[nodiscard]] const std::vector<std::shared_ptr<ExecutionPlanNode>>& children() const noexcept {
        return children_;
    }

    /**
     * @brief Converts to a tree string representation
     */
    [[nodiscard]] std::string to_tree_string(const std::string& prefix = "", bool is_last = true) const {
        std::ostringstream oss;

        // Current node
        oss << prefix;
        oss << (is_last ? "└── " : "├── ");
        oss << operation_type_to_string(type_) << ": " << description_;

        if (estimated_rows_ > 0 || estimated_cost_ > 0.0) {
            oss << " (est. rows: " << estimated_rows_
                << ", cost: " << std::fixed << std::setprecision(2) << estimated_cost_ << ")";
        }

        oss << "\n";

        // Children
        for (std::size_t i = 0; i < children_.size(); ++i) {
            std::string child_prefix = prefix + (is_last ? "    " : "│   ");
            bool child_is_last = (i == children_.size() - 1);
            oss << children_[i]->to_tree_string(child_prefix, child_is_last);
        }

        return oss.str();
    }

private:
    OperationType type_;
    std::string description_;
    std::vector<std::shared_ptr<ExecutionPlanNode>> children_;
    std::size_t estimated_rows_;
    double estimated_cost_;
};

/**
 * @brief Execution plan for a query
 *
 * Visualizes how a query will be executed.
 */
class ExecutionPlan {
public:
    /**
     * @brief Constructs an execution plan
     * @param query_text Original query text/description
     */
    explicit ExecutionPlan(std::string query_text)
        : query_text_(std::move(query_text)), root_(nullptr) {}

    /**
     * @brief Sets the root node of the execution plan
     */
    void set_root(std::shared_ptr<ExecutionPlanNode> root) {
        root_ = std::move(root);
    }

    /**
     * @brief Gets the root node
     */
    [[nodiscard]] const std::shared_ptr<ExecutionPlanNode>& root() const noexcept {
        return root_;
    }

    /**
     * @brief Prints the execution plan
     */
    void print() const {
        std::cout << "=== Query Execution Plan ===\n";
        std::cout << "Query: " << query_text_ << "\n\n";

        if (root_) {
            std::cout << root_->to_tree_string();
        } else {
            std::cout << "(Empty plan)\n";
        }

        std::cout << "\n";
    }

    /**
     * @brief Converts the plan to a string
     */
    [[nodiscard]] std::string to_string() const {
        if (root_) {
            return root_->to_tree_string();
        }
        return "(Empty plan)\n";
    }

private:
    std::string query_text_;
    std::shared_ptr<ExecutionPlanNode> root_;
};

/**
 * @brief Builder for creating execution plans
 */
class ExecutionPlanBuilder {
public:
    /**
     * @brief Starts building an execution plan
     * @param query_text Query description
     */
    explicit ExecutionPlanBuilder(std::string query_text)
        : plan_(std::move(query_text)) {}

    /**
     * @brief Creates a table scan node
     */
    static std::shared_ptr<ExecutionPlanNode> table_scan(
        const std::string& table_name,
        std::size_t estimated_rows = 0)
    {
        auto node = std::make_shared<ExecutionPlanNode>(
            OperationType::TableScan,
            "Table: " + table_name
        );
        node->set_estimated_rows(estimated_rows);
        node->set_estimated_cost(static_cast<double>(estimated_rows));
        return node;
    }

    /**
     * @brief Creates an index scan node
     */
    static std::shared_ptr<ExecutionPlanNode> index_scan(
        const std::string& index_name,
        std::size_t estimated_rows = 0)
    {
        auto node = std::make_shared<ExecutionPlanNode>(
            OperationType::IndexScan,
            "Index: " + index_name
        );
        node->set_estimated_rows(estimated_rows);
        node->set_estimated_cost(static_cast<double>(estimated_rows) * 0.1); // Index is cheaper
        return node;
    }

    /**
     * @brief Creates a filter node
     */
    static std::shared_ptr<ExecutionPlanNode> filter(
        const std::string& condition,
        std::size_t estimated_rows = 0)
    {
        auto node = std::make_shared<ExecutionPlanNode>(
            OperationType::Filter,
            "Condition: " + condition
        );
        node->set_estimated_rows(estimated_rows);
        node->set_estimated_cost(static_cast<double>(estimated_rows) * 0.5);
        return node;
    }

    /**
     * @brief Creates a join node
     */
    static std::shared_ptr<ExecutionPlanNode> join(
        const std::string& join_type,
        const std::string& condition,
        std::size_t estimated_rows = 0)
    {
        auto node = std::make_shared<ExecutionPlanNode>(
            OperationType::Join,
            join_type + " ON " + condition
        );
        node->set_estimated_rows(estimated_rows);
        node->set_estimated_cost(static_cast<double>(estimated_rows) * 2.0);
        return node;
    }

    /**
     * @brief Creates a group-by node
     */
    static std::shared_ptr<ExecutionPlanNode> group_by(
        const std::string& fields,
        std::size_t estimated_rows = 0)
    {
        auto node = std::make_shared<ExecutionPlanNode>(
            OperationType::GroupBy,
            "Fields: " + fields
        );
        node->set_estimated_rows(estimated_rows);
        node->set_estimated_cost(static_cast<double>(estimated_rows) * 1.5);
        return node;
    }

    /**
     * @brief Creates a sort node
     */
    static std::shared_ptr<ExecutionPlanNode> sort(
        const std::string& fields,
        std::size_t estimated_rows = 0)
    {
        auto node = std::make_shared<ExecutionPlanNode>(
            OperationType::Sort,
            "Fields: " + fields
        );
        node->set_estimated_rows(estimated_rows);
        node->set_estimated_cost(static_cast<double>(estimated_rows) * std::log2(estimated_rows + 1));
        return node;
    }

    /**
     * @brief Creates a limit node
     */
    static std::shared_ptr<ExecutionPlanNode> limit(
        std::size_t limit_count)
    {
        auto node = std::make_shared<ExecutionPlanNode>(
            OperationType::Limit,
            "Limit: " + std::to_string(limit_count)
        );
        node->set_estimated_rows(limit_count);
        node->set_estimated_cost(1.0);
        return node;
    }

    /**
     * @brief Creates a projection node
     */
    static std::shared_ptr<ExecutionPlanNode> projection(
        const std::string& fields,
        std::size_t estimated_rows = 0)
    {
        auto node = std::make_shared<ExecutionPlanNode>(
            OperationType::Projection,
            "Fields: " + fields
        );
        node->set_estimated_rows(estimated_rows);
        node->set_estimated_cost(static_cast<double>(estimated_rows) * 0.2);
        return node;
    }

    /**
     * @brief Sets the root of the plan
     */
    ExecutionPlanBuilder& set_root(std::shared_ptr<ExecutionPlanNode> root) {
        plan_.set_root(std::move(root));
        return *this;
    }

    /**
     * @brief Builds and returns the plan
     */
    ExecutionPlan build() {
        return std::move(plan_);
    }

private:
    ExecutionPlan plan_;
};

} // namespace learnql::debug

#endif // LEARNQL_DEBUG_EXECUTION_PLAN_HPP
