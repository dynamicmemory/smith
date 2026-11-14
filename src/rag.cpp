#include <string>
#include <vector>

std::vector<std::string> get_embeddings(std::vector<std::string> text, 
                                        const std::string &model) {

}

std::vector<std::string> fixed_length_chunking(std::vector<std::string> text, 
                                               int chunk_size,
                                               int overlap) {

}

std::vector<std::string> vector_search(const std::string &query,
                                       const std::vector<std::string> &chunks,
                                       const std::vector<std::string> &chunk_embeddings,
                                       int top_k) {

}

void unzip_file_tool();
void list_files_tool();
void read_file_tool();
void read_media_file_tool();

