set -e

echo "🏗️  Building Linux Debug..."
./scripts/build_linux.sh

echo "🏗️  Building Linux Release..."
./scripts/build_linux_release.sh

echo "🏗️  Building Windows Debug..."
./scripts/build_windows.sh

echo "🏗️  Building Windows Release..."
./scripts/build_windows_release.sh

echo "✅  All builds completed successfully!"
