#include "cell.h"

#include <cassert>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <variant>

namespace detail {

	class Impl {
	public:
		using Value = Cell::Value;

		virtual std::string GetText() const = 0;
		virtual Value GetValue(const SheetInterface& sheet) const = 0;

		virtual std::vector<Position> GetReferencedCells() const = 0;
	};

	class EmptyImpl final : public Impl {
	public:
		std::string GetText() const override {
			using namespace std::literals::string_literals;
			return ""s;
		}

		Value GetValue(const SheetInterface& sheet) const override {
			using namespace std::literals::string_literals;
			return ""s;
		}

		std::vector<Position> GetReferencedCells() const override {
			return {};
		}
	};

	class TextImpl final : public Impl {
	public:
		TextImpl(std::string text) : text_(text) {}

		std::string GetText() const override {
			return text_;
		}

		Value GetValue(const SheetInterface&) const override {
			if (!text_.empty() && text_[0] == ESCAPE_SIGN) {
				return text_.substr(1);
			}

			return text_;
		}

		std::vector<Position> GetReferencedCells() const override {
			return {};
		}

	private:
		std::string text_;
	};

	class FormulaImpl final : public Impl {
	public:
		FormulaImpl(std::string expression) : formula_(ParseFormula(expression)) {}

		std::string GetText() const override {
			return FORMULA_SIGN + formula_->GetExpression();
		}

		Value GetValue(const SheetInterface& sheet) const override {
			std::variant<double, FormulaError> res = formula_->Evaluate(sheet);

			if (auto v = std::get_if<double>(&res)) {
				return *v;
			}
			else if (auto v = std::get_if<FormulaError>(&res)) {
				return *v;
			}

			assert(false);
			return {};
		}

		std::vector<Position> GetReferencedCells() const override {
			return formula_->GetReferencedCells();
		}

		bool HasCache() const {
			return formula_->HasCache();
		}
		void ClearCache() {
			formula_->ClearCache();
		}

	private:
		std::unique_ptr<FormulaInterface> formula_;
	};

}  // namespace detail

Cell::Cell(const SheetInterface& sheet, Position pos)
	: sheet_(sheet),
	pos_(pos) {}

Cell::~Cell() {}

void Cell::Set(std::string text) {
	EraseEdges();
	if (text.empty()) {
		//EraseEdges();
		impl_ = std::make_unique<detail::EmptyImpl>();
	}
	else if (text[0] == FORMULA_SIGN && text.size() > 1u) {
		std::unique_ptr<detail::FormulaImpl> formula_impl = std::make_unique<detail::FormulaImpl>(text.substr(1));

		CheckCircularError(formula_impl->GetReferencedCells());
		//EraseEdges();
		AddEdges(formula_impl->GetReferencedCells());

		impl_ = std::move(formula_impl);
	}
	else {
		//EraseEdges();
		impl_ = std::make_unique<detail::TextImpl>(text);
	}
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue(sheet_);
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

void Cell::EraseEdges() {
	// подсчитывание формул у всех клеток сначала
	ClearCaches(referring_cells_);

	for (const Position& cell_pos : referenced_cells_) {
		const CellInterface* cell_interface = sheet_.GetCell(cell_pos);
		if (!cell_interface) {
			continue;
		}

		// удаление данной клетки у reffering cells ссылающейся клетки
		Cell* cell = const_cast<Cell*>(dynamic_cast<const Cell*>(cell_interface));
		cell->referring_cells_.erase(pos_);
	}

	// очищение ссылающихся клеток
	referenced_cells_.clear();
}

void Cell::CheckCircularError(const std::vector<Position>& references) const {
	std::unordered_set<Position, Position::Hash> visited_cells;

	CheckCircularError(pos_, references, visited_cells);
}

void Cell::CheckCircularError(const Position& this_cell_pos, const std::vector<Position>& references,
	std::unordered_set<Position, Position::Hash>& visited_cells) const {
	using namespace std::literals::string_literals;

	for (const Position& ref_cell_pos : references) {
		if (visited_cells.count(ref_cell_pos)) {
			continue;
		}

		// ячейка в цикле нашлась - ошибка!
		if (this_cell_pos == ref_cell_pos) {
			throw CircularDependencyException{ "Circular dependency found"s };
		}
		visited_cells.insert(ref_cell_pos);

		const CellInterface* ref_cell = sheet_.GetCell(ref_cell_pos);
		if (ref_cell == nullptr) {
			continue;
		}

		// проверяем дальше(если клетка существует)
		CheckCircularError(this_cell_pos, ref_cell->GetReferencedCells(), visited_cells);
	}
}

void Cell::ClearCaches(const std::unordered_set<Position, Position::Hash>& reffering_cells) {
	// проходим по каждой клетке, которые ссылаются на данную
	for (const Position& cell_pos : reffering_cells) {
		// аккуратно достаем ссылку на reffering яйчейки
		const Cell* cell = dynamic_cast<const Cell*>(sheet_.GetCell(cell_pos));

		// аккуратно достаем формулу reffering яйчейки
		detail::FormulaImpl* formula_impl = dynamic_cast<detail::FormulaImpl*>(cell->impl_.get());

		// если хеша нет, значит ничего чистить не надо
		if (!formula_impl->HasCache()) {
			continue;
		}

		// хеш есть - удаляем
		formula_impl->ClearCache();

		// чистим дальше(по дереву)
		ClearCaches(cell->referring_cells_);
	}
}

void Cell::AddEdges(const std::vector<Position>& references) {
	using namespace std::literals::string_literals;

	// записываем связи для каждой ячейки,которые включены в формулу
	for (const Position& pos : references) {

		// проверка на существование - если нет, то создаем пустую имплементацию
		if (sheet_.GetCell(pos) == nullptr) {
			SheetInterface& non_const_sheet = const_cast<SheetInterface&>(sheet_);
			non_const_sheet.SetCell(pos, ""s);
		}

		// устанавливаем referenced ячейки
		referenced_cells_.insert(pos);

		// получаем ссылку на существующую ячейку
		const CellInterface* ref_cell_interface = sheet_.GetCell(pos);
		Cell* ref_cell = const_cast<Cell*>(dynamic_cast<const Cell*>(ref_cell_interface));

		// устанавливаем reffering ячейки
		ref_cell->referring_cells_.insert(pos_);
	}
}
