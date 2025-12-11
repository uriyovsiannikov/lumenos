#!/bin/bash

# Пути к папкам
projectDir="/home/uriy/lumenos"
backupRoot="/home/uriy/backups"

# Проверяем существование папки проекта
if [ ! -d "$projectDir" ]; then
    echo "Папка проекта не найдена: $projectDir"
    read -p "Нажмите Enter для продолжения..."
    exit 1
fi

# Создаем папку backups, если ее нет
if [ ! -d "$backupRoot" ]; then
    mkdir "$backupRoot"
    echo "Создана папка для резервных копий: $backupRoot"
fi

# Ищем последний номер бекапа
lastBackup=0
for dir in "$backupRoot"/*/; do
    if [ -d "$dir" ]; then
        folderName=$(basename "$dir")
        # Проверяем, что имя папки состоит только из цифр
        if [[ "$folderName" =~ ^[0-9]+$ ]]; then
            if [ "$folderName" -gt "$lastBackup" ]; then
                lastBackup=$folderName
            fi
        fi
    fi
done

# Увеличиваем номер на 1
newBackup=$((lastBackup + 1))

# Создаем папку для нового бекапа
backupDir="$backupRoot/$newBackup"
mkdir "$backupDir"

# Копируем файлы
echo "Создание резервной копии $newBackup..."
cp -r "$projectDir"/* "$backupDir"/

echo "Резервная копия успешно создана в $backupDir"
read -p "Нажмите Enter для продолжения..."
