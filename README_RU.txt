Замените в корне проекта файл CI/before_script.msvc.sh и обязательно сделайте commit + push.

Контрольные строки в новом GitHub Actions логе:
  Installing aqtinstall 3.3.0 into virtualenv...
  python -m aqt install-qt

Если снова видно "Installing aqt wheel into virtualenv...", workflow собирает старый commit/ветку.
