set -e

echo "🏗️  Building Linux Debug..."
./scripts/build_linux.sh

echo "🏗️  Building Windows Debug..."
./scripts/build_windows.sh

echo "✅  All builds completed successfully!"
