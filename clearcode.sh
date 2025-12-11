#!/bin/bash
process_file() {
    local file="$1"
    local temp_file=$(mktemp)
    echo "Обрабатываю файл: $file"
    local original_perms=$(stat -c "%a" "$file")
    sed -e 's|//.*||' -e '/^[[:space:]]*$/d' "$file" > "$temp_file"
    if ! cmp -s "$file" "$temp_file"; then
        cat "$temp_file" > "$file"
        chmod "$original_perms" "$file"
        echo "  ✓ Файл обновлен: $file (права: $original_perms)"
    else
        echo "  ⨯ Файл не изменился: $file"
    fi
    rm "$temp_file"
}
find_and_process_files() {
    local dir="${1:-.}"
    
    find "$dir" -type f \( -name "*.c" -o -name "*.h" \) | while read -r file; do
        if file "$file" | grep -q "text"; then
            process_file "$file"
        else
            echo "  ⚠ Пропускаем бинарный файл: $file"
        fi
    done
}
main() {
    local target_dir="${1:-.}"
    echo "=== Удаление комментариев и пустых строк ==="
    echo "Целевая директория: $target_dir"
    echo ""
    if [ ! -d "$target_dir" ]; then
        echo "Ошибка: Директория '$target_dir' не существует!"
        exit 1
    fi
    file_count=$(find "$target_dir" -type f \( -name "*.c" -o -name "*.h" \) | wc -l)
    echo "Найдено файлов для обработки: $file_count"
    echo ""
    read -p "Продолжить? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Отменено пользователем."
        exit 0
    fi
    find_and_process_files "$target_dir"
    echo ""
    echo "=== Обработка завершена ==="
}
check_dependencies() {
    local deps=("find" "sed" "file" "cmp" "stat")
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            echo "Ошибка: Не найдена утилита '$dep'"
            exit 1
        fi
    done
}
check_dependencies
main "$@"
