#include <filesystem>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include "openssl/md5.h"

using namespace std;
using namespace filesystem;

using char_t = unsigned char;
using str_t = basic_string<char_t>;
using ifstream_t = basic_ifstream<char_t>;

str_t to_md5(ifstream_t& is)
{
	MD5_CTX md5_ctx;
	MD5_Init(&md5_ctx);

	constexpr auto buf_sz = 1024;
	char_t buf[buf_sz];
	streamsize bytes;

	while ((bytes = is.readsome(buf, buf_sz)) != 0)
		MD5_Update(&md5_ctx, buf, bytes);

	str_t hash(MD5_DIGEST_LENGTH, 0);
	MD5_Final(hash.data(), &md5_ctx);

	return hash;
}

int main(const int argc, char* argv[])
{
	if (argc < 3)
	{
		cout << "Usage: duprm <TARGET_DIR> <MOVE_DIR>\n";
		return 0;
	}
	
	const path target_dir{argv[1]}, move_dir{argv[2]};
	const auto target_dir_len = strlen(argv[1]);
	if (!exists(target_dir))
	{
		cout << "Target dir " << target_dir << " not exists";
		return 0;
	}

	vector<path> files;

	for (auto&& x : recursive_directory_iterator{target_dir})
	{
		if (x.is_regular_file()) files.push_back(x.path());
	}

	sort(files.begin(), files.end(), [](const path& aa, const path& bb)
	{
		const wstring_view a{aa.c_str()}, b{bb.c_str()};
		if (a.size() == b.size()) return a < b;
		return a.size() < b.size();
	});
	
	unordered_set<str_t> hashes;

	for (const auto& x : files)
	{
		ifstream_t file{x, ios_base::binary};
		if (!file) continue;

		const auto md5 = to_md5(file);
		file.close();
		
		auto [pos, has_inserted] = hashes.insert(md5);
		if (has_inserted) continue;

		auto new_path_str = x.string();
		new_path_str.replace(0, target_dir_len, move_dir.string());
		
		try
		{
			path new_path{new_path_str};
			const auto new_dir = new_path.parent_path();
			
			if (!exists(new_dir)) create_directories(new_dir);
			rename(x, new_path);
			
			const auto old_dir = x.parent_path();
			if (filesystem::is_empty(old_dir)) remove(old_dir);
			
			cout << x.filename() << " >>> " << new_dir << '\n';
		}
		catch (exception& e)
		{
			cout << "ERROR: " << e.what() << '\n';
		}
	}
}
