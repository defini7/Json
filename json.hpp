#ifndef DEF_JSON_HPP
#define DEF_JSON_HPP

#include <string_view>
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdarg>
#include <fstream>
#include <sstream>

namespace Json
{
    namespace Detail
    {
        namespace Utils
        {
            void LogError(const char* format, ...);
            bool ReadFile(const std::string_view fileName, std::string& output);
        }

        enum class StateType
        {
            New,
            Null,
            Numeric,
            Boolean,
            String,
            Complete,
            LeftParen,
            RightParen,
            LeftBracket,
            RightBracket
        };

        enum class TokenType
        {
            Undefined,
            Numeric,
            Boolean,
            String,
            Null,

            OpLeftBrace,
            OpRightBrace,
            OpLeftBracket,
            OpRightBracket,
            OpComma,
            OpColon
        };

        struct Token
        {
            TokenType Type;
            std::string Value;

            void Print();
        };

        std::string TokenTypeToString(TokenType type);

        bool IsDigit(char c);
        bool IsAlpha(char c);
        bool IsNewline(char c);
        bool IsWhitespace(char c);
        bool IsQuote(char c);
        bool IsLeftParen(char c);
        bool IsRightParen(char c);
        bool IsBooleanChar(char c);
        bool IsNullChar(char c);

        class Lexer
        {
        public:
            Lexer() = default;

            void Reset(const std::string_view raw);

            // Returns true if we've got a token and returns false in a case of invalid token
            // or an end of the file
            bool NextToken(Token& token);

        private:
            void StartToken(Token& token, TokenType type, StateType nextState, bool push = true);
            void AppendChar(Token& token, StateType nextState);

            bool CheckEof();
            bool SkipWhitespaces();

        private:
            std::string m_Input;

            size_t m_Cursor;
            size_t m_Line;
            size_t m_LineStart;

            StateType m_CurrentState;
            StateType m_NextState;

            int m_ParenthesisBalancer;
            int m_QuotesBalancer;

        };
    }

    template <class T>
    struct OrderedUnorderedMap
    {
        std::unordered_map<std::string, size_t> Indecies;
        std::vector<T> Data;

        T& operator[](const std::string& key);
    };

    template <class T>
    using ScopedPtr = std::unique_ptr<T>;

    struct Node
    {
        friend class Parser;

        enum class Kind
        {
            Null,
            String,
            Boolean,
            Number,
            Array,
            Object
        };

        using ObjectType = OrderedUnorderedMap<ScopedPtr<Node>>;
        using ArrayType = std::vector<ScopedPtr<Node>>;

        union ValueType
        {
            std::string* String;
            bool* Boolean;
            double* Number;
            ArrayType* Array;
            ObjectType* Object;
        };

        void Dump(std::ostream& output = std::cout, size_t offset = 0, size_t tabSize = 4) const;

        Node& operator[](const std::string& key);
        Node& operator[](size_t index);

        std::string& String();
        bool& Bool();
        double& Number();

        bool IsArray() const;
        bool IsObject() const;
        bool IsString() const;
        bool IsBool() const;
        bool IsNumber() const;
        bool IsNull() const;

    protected:
        ValueType m_Value;
        Kind m_Type;
    };

    std::string Dump(const Node& node, size_t tabSize = 4);

    class Parser
    {
    public:
        Parser() = default;

        friend struct Node;

        void Reset(std::string_view raw);
        void Parse(Node& root);

    private:
        bool NextToken();

        // If the current token is of type "type" then grabs the next token
        // otherwise prints an error
        bool Expect(Detail::TokenType type);

        void ParseAtom(Node& node);
        void ParseValue(Node& node);
        void ParseAssignment(Node& node);
        void ParseObject(Node& node);
        void ParseArray(Node& node);

    private:
        Detail::Token m_CurrentToken;
        Detail::Lexer m_Lexer;

    };

    Node ParseFile(const std::string_view fileName);
    Node ParseRaw(const std::string_view raw);

#ifdef JSON_IMPL
#undef JSON_IMPL

    void Detail::Utils::LogError(const char* format, ...)
    {
        va_list args;

        printf("[JSON] ");

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }

    bool Detail::Utils::ReadFile(const std::string_view fileName, std::string& output)
    {
        std::ifstream ifs(fileName.data());

        if (!ifs.is_open())
            return false;

        int c;
        while ((c = ifs.get()) != EOF)
            output.push_back(char(c));

        return true;
    }

    std::string Detail::TokenTypeToString(TokenType type)
    {
        switch (type)
        {
        case TokenType::Undefined:      return "TokenType::Undefined";
        case TokenType::Numeric:        return "TokenType::Numeric";
        case TokenType::String:         return "TokenType::String";
        case TokenType::Boolean:        return "TokenType::Boolean";
        case TokenType::Null:           return "TokenType::Null";
        case TokenType::OpLeftBracket:  return "TokenType::OpLeftBracket";
        case TokenType::OpRightBracket: return "TokenType::OpRightBracket";
        case TokenType::OpLeftBrace:    return "TokenType::OpLeftBrace";
        case TokenType::OpRightBrace:   return "TokenType::OpRightBrace";
        case TokenType::OpComma:        return "TokenType::OpComma";
        case TokenType::OpColon:        return "TokenType::OpColon";
        }

        return "";
    }

    void Detail::Token::Print()
    {
        std::string typeStr = TokenTypeToString(Type);
        printf("[%s] %s\n", typeStr.c_str(), Value.c_str());
    }

    bool Detail::IsDigit(char c)
    {
        return '0' <= c && c <= '9' || c == '.';
    }

    bool Detail::IsAlpha(char c)
    {
        return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z';
    }

    bool Detail::IsNewline(char c)
    {
        return c == '\r' || c == '\n';
    }

    bool Detail::IsWhitespace(char c)
    {
        return c == ' ' || c == '\t';
    }

    bool Detail::IsQuote(char c)
    {
        return c == '\'' || c == '"';
    }

    bool Detail::IsLeftParen(char c)
    {
        return c == '[' || c == '{';
    }

    bool Detail::IsRightParen(char c)
    {
        return c == ']' || c == '}';
    }

    bool Detail::IsBooleanChar(char c)
    {
        return c == 't' || c == 'r' || c == 'u' || c == 'e' ||
            c == 'f' || c == 'a' || c == 'l' || c == 's';
    }

    bool Detail::IsNullChar(char c)
    {
        return c == 'n' || c == 'u' || c == 'l';
    }

    void Detail::Lexer::StartToken(Token& token, TokenType type, StateType nextState, bool push)
    {
        token.Type = type;

        if (push)
            token.Value.append(1, m_Input[m_Cursor]);

        m_NextState = nextState;
    }

    void Detail::Lexer::AppendChar(Token& token, StateType nextState)
    {
        token.Value.append(1, m_Input[m_Cursor]);

        m_NextState = nextState;
        m_Cursor++;
    }

    bool Detail::Lexer::CheckEof()
    {
        if (m_Cursor >= m_Input.length())
        {
            if (m_ParenthesisBalancer != 0)
                Utils::LogError("parentheses were not balanced\n");

            if (m_QuotesBalancer != 0)
                Utils::LogError("quotes were not balanced\n");

            return true;
        }

        return false;
    }

    bool Detail::Lexer::SkipWhitespaces()
    {
        bool eof;

        while (!(eof = CheckEof()) && (
            IsWhitespace(m_Input[m_Cursor]) ||
            IsNewline(m_Input[m_Cursor])))
            m_Cursor++;

        return !eof;
    }

    void Detail::Lexer::Reset(const std::string_view raw)
    {
        m_Input = raw;
        m_CurrentState = StateType::New;
        m_NextState = StateType::New;
        m_Cursor = 0;
        m_Line = 0;
        m_LineStart = 0;
        m_ParenthesisBalancer = 0;
        m_QuotesBalancer = 0;
    }

    bool Detail::Lexer::NextToken(Token& token)
    {
        if (m_Input.empty())
            return false;

        token.Type = TokenType::Undefined;
        token.Value.clear();

        if (CheckEof())
            return false;

        auto push = [&]()
            {
                token.Value.append(1, m_Input[m_Cursor]);
            };

        auto cur = [&]()
            {
                return m_Input.at(m_Cursor);
            };

        auto complete = [&](TokenType type, bool push)
            {
                StartToken(token, type, StateType::Complete, push);
            };

        do
        {
            if (m_CurrentState == StateType::New)
            {
                if (!SkipWhitespaces())
                    return false;

                if (IsDigit(cur()))
                {
                    StartToken(
                        token,
                        TokenType::Numeric,
                        StateType::Numeric);
                }

                else if (IsQuote(cur()))
                {
                    StartToken(
                        token,
                        TokenType::String,
                        StateType::String,
                        false
                    );

                    m_QuotesBalancer++;
                }

                else if (cur() == 't' || cur() == 'f')
                {
                    StartToken(
                        token,
                        TokenType::Boolean,
                        StateType::Boolean);
                }

                else if (cur() == 'n')
                {
                    StartToken(
                        token,
                        TokenType::Null,
                        StateType::Null
                    );
                }

                else if (IsLeftParen(cur()))
                {
                    complete(cur() == '[' ? TokenType::OpLeftBracket : TokenType::OpLeftBrace, true);
                    m_ParenthesisBalancer++;
                }

                else if (IsRightParen(cur()))
                {
                    complete(cur() == ']' ? TokenType::OpRightBracket : TokenType::OpRightBrace, true);
                    m_ParenthesisBalancer--;
                }

                else if (cur() == ',')
                    complete(TokenType::OpComma, false);

                else if (cur() == ':')
                    complete(TokenType::OpColon, false);

                else
                {
                    Utils::LogError("unexpected character %c\n", cur());
                    return false;
                }

                m_Cursor++;
            }
            else
            {
                switch (m_CurrentState)
                {
                case StateType::Numeric:
                {
                    if (IsDigit(cur()))
                        AppendChar(token, StateType::Numeric);
                    else
                    {
                        if (IsAlpha(cur()))
                        {
                            push();
                            Utils::LogError("invalid numeric literal or symbol: %s\n", token.Value.c_str());
                            return false;
                        }

                        m_NextState = StateType::Complete;
                    }
                }
                break;

                case StateType::String:
                {
                    if (IsQuote(cur()))
                    {
                        m_QuotesBalancer--;
                        m_NextState = StateType::Complete;
                        m_Cursor++;
                    }
                    else
                        AppendChar(token, StateType::String);
                }
                break;

                case StateType::Boolean:
                {
                    if (IsBooleanChar(cur()))
                        AppendChar(token, StateType::Boolean);
                    else
                    {
                        if (token.Value == "true" || token.Value == "false")
                            token.Type = TokenType::Boolean;
                        else
                        {
                            Utils::LogError("unexpected symbol: %s\n", token.Value.c_str());
                            return false;
                        }

                        m_NextState = StateType::Complete;
                    }
                }
                break;

                case StateType::Null:
                {
                    if (IsNullChar(cur()))
                        AppendChar(token, StateType::Null);
                    else
                    {
                        if (token.Value == "null")
                            token.Type = TokenType::Null;
                        else
                        {
                            Utils::LogError("unexpected symbol: %s\n", token.Value.c_str());
                            return false;
                        }

                        m_NextState = StateType::Complete;
                    }
                }
                break;

                case StateType::Complete:
                    m_NextState = StateType::New;
                break;

                }
            }

            m_CurrentState = m_NextState;
        }
        while (m_NextState != StateType::New);

        return true;
    }

    void Node::Dump(std::ostream& output, size_t offset, size_t tabSize) const
    {
        auto PrintTabs = [&](size_t spaces)
            {
                for (size_t i = 0; i < spaces; i++)
                    output.put(' ');
            };

        PrintTabs(offset);

        switch (m_Type)
        {
        case Kind::Object:
        {
            output << "{\n";

            auto& obj = *m_Value.Object;

            for (auto& [name, index] : obj.Indecies)
            {
                auto& value = *obj.Data[index];

                PrintTabs(offset + tabSize);
                output << '"' << name << "\": ";

                if (value.m_Type == Kind::Object || value.m_Type == Kind::Array)
                {
                    output << "\n";
                    value.Dump(output, offset + tabSize, tabSize);
                }
                else
                    value.Dump(output, 0, tabSize);

                output << "\n";
            }

            PrintTabs(offset);
            output << "},";
        }
        break;

        case Kind::Boolean:
            output << std::boolalpha << *m_Value.Boolean << ',';
        break;

        case Kind::Number:
            output << *m_Value.Number << ',';
        break;

        case Kind::String:
            output << '"' << *m_Value.String << "\",";
        break;

        case Kind::Null:
            output << "null";
        break;

        case Kind::Array:
        {
            output << "[\n";

            for (const auto& value : *m_Value.Array)
            {
                value->Dump(output, offset + tabSize, tabSize);
                output.put('\n');
            }

            PrintTabs(offset);
            output << "],";
        }
        break;

        }
    }

    Node& Node::operator[](const std::string& key)
    {
        if (IsObject())
            return *(*m_Value.Object)[key];

        throw std::out_of_range("Bad object key");
    }

    Node& Node::operator[](size_t index)
    {
        if (IsArray())
            return *(*m_Value.Array)[index];

        throw std::out_of_range("Array index subscript is out of range");
    }

    std::string& Node::String()
    {
        if (IsString())
            return *m_Value.String;

        throw std::bad_cast();
    }

    bool& Node::Bool()
    {
        if (IsBool())
            return *m_Value.Boolean;

        throw std::bad_cast();
    }

    double& Node::Number()
    {
        if (IsNumber())
            return *m_Value.Number;

        throw std::bad_cast();
    }

    bool Node::IsArray() const
    {
        return m_Type == Kind::Array;
    }

    bool Node::IsObject() const
    {
        return m_Type == Kind::Object;
    }

    bool Node::IsString() const
    {
        return m_Type == Kind::String;
    }

    bool Node::IsBool() const
    {
        return m_Type == Kind::Boolean;
    }

    bool Node::IsNumber() const
    {
        return m_Type == Kind::Number;
    }

    bool Node::IsNull() const
    {
        return m_Type == Kind::Null;
    }

    void Parser::Reset(std::string_view raw)
    {
        m_Lexer.Reset(raw);

        m_CurrentToken.Type = Detail::TokenType::Undefined;
        m_CurrentToken.Value.clear();
    }

    void Parser::Parse(Node& root)
    {
        NextToken();
        ParseObject(root);
    }

    bool Parser::NextToken()
    {
        return m_Lexer.NextToken(m_CurrentToken);
    }

    bool Parser::Expect(Detail::TokenType type)
    {
        if (m_CurrentToken.Type != type)
        {
            std::string expected = TokenTypeToString(type);
            std::string actual = TokenTypeToString(type);

            Detail::Utils::LogError(
                "expected %s but got %s\n",
                expected.c_str(), actual.c_str());

            return false;
        }

        NextToken();
        return true;
    }

    void Parser::ParseAtom(Node& node)
    {
        using TT = Detail::TokenType;

        if (m_CurrentToken.Type == TT::Boolean)
        {
            node.m_Type = Node::Kind::Boolean;
            node.m_Value.Boolean = new bool(m_CurrentToken.Value == "true");
        }

        else if (m_CurrentToken.Type == TT::String)
        {
            node.m_Type = Node::Kind::String;
            node.m_Value.String = new std::string(m_CurrentToken.Value);
        }

        else if (m_CurrentToken.Type == TT::Numeric)
        {
            node.m_Type = Node::Kind::Number;
            node.m_Value.Number = new double(std::stod(m_CurrentToken.Value));
        }

        else if (m_CurrentToken.Type == TT::Null)
            node.m_Type = Node::Kind::Null;
        
        else
        {
            std::string type = Detail::TokenTypeToString(m_CurrentToken.Type);
            Detail::Utils::LogError("unexpected token type: %s\n", type.c_str());

            return;
        }

        NextToken();
    }

    void Parser::ParseValue(Node& node)
    {
        using TT = Detail::TokenType;

        switch (m_CurrentToken.Type)
        {
        case TT::OpLeftBrace: ParseObject(node); break;
        case TT::OpLeftBracket: ParseArray(node); break;
        default: ParseAtom(node);
        }
    }

    void Parser::ParseAssignment(Node& node)
    {
        std::string key = m_CurrentToken.Value;

        NextToken();
        Expect(Detail::TokenType::OpColon);

        auto& valueNode = (*node.m_Value.Object)[key];
        valueNode = std::make_unique<Node>();

        ParseValue(*valueNode);
    }

    void Parser::ParseObject(Node& node)
    {
        using TT = Detail::TokenType;

        Expect(TT::OpLeftBrace);

        node.m_Type = Node::Kind::Object;
        node.m_Value.Object = new Node::ObjectType();

        ParseAssignment(node);

        while (m_CurrentToken.Type == TT::OpComma)
        {
            NextToken();
            ParseAssignment(node);
        }

        Expect(TT::OpRightBrace);
    }

    void Parser::ParseArray(Node& node)
    {
        using TT = Detail::TokenType;

        node.m_Type = Node::Kind::Array;
        node.m_Value.Array = new Node::ArrayType();

        do
        {
            NextToken();

            node.m_Value.Array->push_back(std::make_unique<Node>());

            auto& valueNode = node.m_Value.Array->back();
            ParseValue(*valueNode);
        }
        while (m_CurrentToken.Type == TT::OpComma);

        Expect(TT::OpRightBracket);
    }

    std::string Dump(const Node& node, size_t tabSize)
    {
        std::stringstream ss;
        node.Dump(ss, 0, tabSize);

        return std::string(ss.rdbuf()->str());
    }

    Node ParseFile(const std::string_view fileName)
    {
        std::string raw;
        Detail::Utils::ReadFile(fileName, raw);

        return ParseRaw(raw);
    }

    Node ParseRaw(const std::string_view raw)
    {
        Parser parser;
        Node root;

        parser.Reset(raw);
        parser.Parse(root);

        return root;
    }

    template<class T>
    T& OrderedUnorderedMap<T>::operator[](const std::string& key)
    {
        if (Indecies.count(key) == 0)
        {
            Indecies[key] = Data.size();
            Data.push_back(T());

            return Data.back();
        }

        return Data[Indecies[key]];
    }

#endif
}

#endif
